#include "TrackData.h"

void BamTrackData::updateRegion()
{
	if (!bam_reader_)
	{
		qWarning() << "Bam Reader is not set For BamTrackData" << Qt::endl;
		return;
	}

	int max_len = SharedData::settings().bam_max_region_len;
	const BedLine& region = SharedData::region();

	alignments_.clear();

	if (region.length() > max_len)
	{
		bam_reader_->setRegion(region.chr(), 0, 0);
		emit onDataUpdate();
		return;
	}

	int padding = region.length();
	int start = region.start() - padding;

	bam_reader_->setRegion(region.chr(), start, region.end());
	updateData();
}


void BamTrackData::updateData()
{
	alignments_.clear();
	BamAlignment al;
	while (bam_reader_->getNextAlignment(al))
	{
		if (al.isUnmapped() || al.isDuplicate() || al.isSecondaryAlignment()) continue;
		alignments_ << al;
	}

	// std::sort(alignments_.begin(), alignments_.end(), [](const BamAlignment& a, const BamAlignment& b) {
	// 	return a.start() < b.start();  // Compare the start positions of two alignments
	// });

	const BedLine& region = SharedData::region();
	ref_seq_ = SharedData::genome().seq(region.chr(), region.start(), region.length());
	emit onDataUpdate();
}
