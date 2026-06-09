#include "BamTrackData.h"
#include "FileLoader.h"

#define BAM_OPTIMIZATION

AlignmentKey AlignmentKey::makeKey(const BamAlignment& al)
{
	QByteArray cigar = al.cigarDataAsString();

	uint64_t h1 = qHash(al.name());
	h1 ^= ((uint64_t)al.start() << 1);
	h1 ^= ((uint64_t)al.isReverseStrand() << 32);

	uint64_t h2 = qHashBits(cigar.data(), cigar.size());

	return {h1, h2};
}


void BamTrackData::updateRegion()
{
	if (!bam_reader_)
	{
		qWarning() << "Bam Reader is not set For BamTrackData" << Qt::endl;
		return;
	}

	int max_len = SharedData::settings().bam_max_region_len;
	const BedLine& region = SharedData::region();

	if (region.length() > max_len)
	{
		bam_reader_->setRegion(region.chr(), 0, 0);
		return_empty_ = true;
		emit onDataUpdate();
		return;
	}

	return_empty_ = false;

	#ifdef BAM_OPTIMIZATION

	int new_start = region.start();
	int new_end = region.end();
	int old_start = loaded_region_.start();
	int old_end = loaded_region_.end();

	if (!loaded_region_.isValid() || region.chr() != loaded_region_.chr()) // chr mismatch
	{
		fullLoad(region);
		return;
	}

	else if (new_start >= old_start && new_end <= old_end) // already loaded
	{
		emit onDataUpdate();
		return;
	}

	else if (new_end < old_start || new_start > old_end) // completely outside of bounds
	{
		fullLoad(region);
		return;
	}
	else // left/right shift or size increase
	{
		is_loading_ = true;

		int padding = region.length() / 3;

		int p_start = std::max(0, new_start - padding);
		int p_end = new_end + padding;

		pruneAlignments(p_start, p_end); // remove out of bound alignments

		ref_seq_ = SharedData::genome().seq(region.chr(), p_start, p_end - p_start + 1);
		ref_start_ = p_start;

		if (p_start < old_start) fetchRegion({loaded_region_.chr(), p_start, old_start});
		if (p_end > old_end) fetchRegion({loaded_region_.chr(), old_end, p_end});

		loaded_region_.setStart(p_start);
		loaded_region_.setEnd(p_end);
		is_loading_ = false;

		emit onDataUpdate();
		return;
	}


	#else
	alignments_.clear();
	return_empty_ = false;


	int padding = region.length() / 3;
	int start = region.start() - padding;
	int end = region.end() + padding;

	bam_reader_->setRegion(region.chr(), start, end);

	updateData();

	#endif
}

void BamTrackData::fullLoad(const BedLine& region)
{
	is_loading_ = true;
	int padding = region.length() / 3;
	int p_start = std::max(0, region.start() - padding);
	int p_end = region.end() + padding;

	alignments_.clear();

	ref_seq_ = SharedData::genome().seq(region.chr(), p_start, p_end - p_start + 1);
	ref_start_ = p_start;

	BedLine region_to_fetch(region.chr(), p_start, p_end);

	fetchRegion(region_to_fetch);

	loaded_region_.setChr(region.chr());
	loaded_region_.setStart(p_start);
	loaded_region_.setEnd(p_end);

	is_loading_ = false;

	emit onDataUpdate();
}

void BamTrackData::fetchRegion(const BedLine& region)
{
	bam_reader_->setRegion(region.chr(), region.start(), region.end());

	BamAlignment al;
	while (bam_reader_->getNextAlignment(al))
	{
		if (al.isUnmapped() || al.isDuplicate() || al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;

		BamAlignmentWrapper wrapped_alignment(al);
		if (loaded_ids_.contains(wrapped_alignment.id)) continue;
		loaded_ids_.insert(wrapped_alignment.id);

		// wrapped_alignment.storeMismatches(ref_seq_, ref_start_);
		wrapped_alignment.storeCigarData(ref_seq_, ref_start_);
		alignments_.push_back(std::move(wrapped_alignment));
	}
}

void BamTrackData::pruneAlignments(int keep_start, int keep_end)
{
	auto it = std::remove_if(
		alignments_.begin(),
		alignments_.end(),
		[&](const BamAlignmentWrapper& al)
		{
			bool remove =
				al.alignment.start() > keep_end ||
				al.alignment.end() < keep_start;

			// also remove the id from loaded_ids_
			if (remove) loaded_ids_.remove(al.id);

			return remove;
		});

	alignments_.erase(it, alignments_.end());
}

void BamAlignmentWrapper::storeCigarData(const Sequence& ref_seq, int ref_start)
{
	events.clear();
	mismatches.clear();

	int read_pos =0;
	int genome_pos = alignment.start();

	CigarData cigar = alignment.cigarData();

	events.reserve(cigar.size());

	for (uint32_t i =0; i < cigar.size(); ++i)
	{
		EventData c_data;
		uint32_t op = cigar.opType(i);
		uint32_t len = cigar.opLength(i);

		c_data.length = len;
		c_data.genome_pos = genome_pos;

		switch (op)
		{
			case BAM_CMATCH:
			case BAM_CEQUAL:
			case BAM_CDIFF:
				c_data.event = MATCH;
				break;
			case BAM_CINS:
				c_data.event = INSERTION;
				break;
			case BAM_CDEL:
				c_data.event = DELETION;
				genome_pos += len;
				break;
			case BAM_CSOFT_CLIP:
				read_pos += len;
				continue;
			case BAM_CREF_SKIP:
				genome_pos += len;
				continue;
			case BAM_CHARD_CLIP:
				continue;
			default: // TODO: handle exception (invalid cigar op)
				continue;
		}
		if (c_data.event == MATCH)
		{
			for (uint32_t j =0; j < len; ++j)
			{
				char base = alignment.base(read_pos) | 32;
				int qual = alignment.quality(read_pos);
				c_data.bases.push_back(base);
				c_data.qualities << qual;
				int idx = genome_pos - ref_start;
				if (idx >= 0 && idx < ref_seq.length())
				{
					char ref = ref_seq[idx] | 32;
					// mismatches are stored seperatly so the draw function is more optimized
					// otherwise for each draw all bases would need to be checked
					if (ref != base) mismatches.push_back({genome_pos, base, qual});
				}
				++genome_pos;
				++read_pos;
			}
		}
		else if (c_data.event == INSERTION)
		{
			for (uint32_t j =0; j < len; ++j)
			{
				char base = alignment.base(read_pos) | 32;
				int qual = alignment.quality(read_pos);
				c_data.bases.append(base);
				c_data.qualities << qual;
				++read_pos;
			}
		}
		events << c_data;
	}
}

void BamTrackData::updateData()
{
	alignments_.clear();
	const BedLine& region = SharedData::region();
	ref_seq_ = SharedData::genome().seq(region.chr(), region.start(), region.length());
	ref_start_ = region.start();
	BamAlignment al;
	while (bam_reader_->getNextAlignment(al))
	{
		if (al.isUnmapped() || al.isDuplicate() || al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
		BamAlignmentWrapper wrapped_alignment(al);
		wrapped_alignment.storeCigarData(ref_seq_, region.start());
		alignments_ << wrapped_alignment;
	}
	emit onDataUpdate();
}

void BamTrackData::reload()
{
	if (is_loading_) return;
	is_loading_ = true;

	QSharedPointer<BamReader> reader = FileLoader::loadBamFile(file_path_);
	if (reader) setBamReader(reader);

	is_loading_ = false;
}
