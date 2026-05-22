#include "TrackData.h"
#include "FileLoader.h"

#define OPT
#undef OPT

void BamTrackData::updateRegion()
{
	if (!bam_reader_)
	{
		qWarning() << "Bam Reader is not set For BamTrackData" << Qt::endl;
		return;
	}

	// if (is_loading_) return;

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

	#ifdef OPT

	if (!loaded_region_.isValid() || region.chr() != loaded_region_.chr())
	{
		qDebug() << "going into full load" << Qt::endl;
		fullLoad(region);
		emit onDataUpdate();
		qDebug() << "returning from full load" << Qt::endl;
		return;
	}

	int new_start = region.start();
	int new_end = region.end();
	int old_start = loaded_region_.start();
	int old_end = loaded_region_.end();

	if (new_start >= old_start && new_end <= old_end) // already loaded
	{
		qDebug() << "Zoomed in giving, pre loaded stuff" << Qt::endl;
		emit onDataUpdate();
		return;
	}

	if (new_end < old_start || new_start > old_end)
	{
		qDebug() << "out of loaded region, reloading everything" << Qt::endl;
		fullLoad(region);
		emit onDataUpdate();
		return;
	}

	qDebug() << "left or right shift" << Qt::endl;

	is_loading_ = true;

	int padding = region.length() / 3;

	int p_start = std::max(0, new_start - padding);
	int p_end = new_end + padding;

	ref_seq_ = SharedData::genome().seq(region.chr(), p_start, p_end - p_start + 1);
	ref_start_ = p_start;

	if (new_start < old_start) fetchRegion({loaded_region_.chr(), p_start, old_start}, p_start);
	if (new_end > old_end) fetchRegion({loaded_region_.chr(), old_end, p_end}, p_start);

	pruneAlignments(p_start, p_end);

	loaded_region_.setStart(p_start);
	loaded_region_.setEnd(p_end);
	is_loading_ = false;
	emit onDataUpdate();

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
	qDebug() << "Initiating full load" << Qt::endl;
	int padding = region.length() / 3;
	int p_start = std::max(0, region.start() - padding);
	int p_end = region.end() + padding;

	alignments_.clear();

	ref_seq_ = SharedData::genome().seq(region.chr(), p_start, p_end - p_start + 1);
	ref_start_ = p_start;

	BedLine region_to_fetch(region.chr(), p_start, p_end);

	fetchRegion(region_to_fetch, p_start);

	loaded_region_.setChr(region.chr());
	loaded_region_.setStart(p_start);
	loaded_region_.setEnd(p_end);

	is_loading_ = false;
	qDebug() << "Finished full load" << Qt::endl;
}

void BamTrackData::fetchRegion(const BedLine& region, int ref_start)
{
	// qDebug() << "Fetching region: " << region.start() <<' ' << region.end() << Qt::endl;
	QSet<QString> existing;
	existing.reserve(alignments_.size());

	foreach (const BamAlignmentWrapper& aln, alignments_) existing.insert(aln.id);

	bam_reader_->setRegion(region.chr(), region.start(), region.end());

	BamAlignment al;
	while (bam_reader_->getNextAlignment(al))
	{
		if (al.isUnmapped() || al.isDuplicate() || al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;

		BamAlignmentWrapper wrapped_alignment(al);
		if (existing.contains(wrapped_alignment.id)) continue;

		wrapped_alignment.storeVariants(ref_seq_, ref_start);
		alignments_ << wrapped_alignment;
	}
	// qDebug() << "Fetched region: " << region.start() <<' ' << region.end() << Qt::endl;
}

void BamTrackData::pruneAlignments(int keep_start, int keep_end)
{
	alignments_.erase(
		std::remove_if(alignments_.begin(), alignments_.end(), [keep_start, keep_end](const BamAlignmentWrapper& al){
			return (al.alignment.start() > keep_end || al.alignment.end() < keep_start);
		} ), alignments_.end()
		);
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
		wrapped_alignment.storeVariants(ref_seq_, region.start());
		alignments_ << wrapped_alignment;
	}
	emit onDataUpdate();
}

void BamTrackData::reload()
{
	if (is_loading_) return;
	is_loading_ = true;

	QSharedPointer<BamReader> reader = FileLoader::loadBamFile(file_path);
	if (reader) setBamReader(reader);

	is_loading_ = false;
}
