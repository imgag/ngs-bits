#include "WorkerLowCoverage.h"
#include "BamReader.h"

WorkerLowCoverage::WorkerLowCoverage(Chunk& chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file)
	: QRunnable()
	, chunk_(chunk)	
	, bam_file_(bam_file)
	, cutoff_(cutoff)
	, min_mapq_(min_mapq)
	, min_baseq_(min_baseq)
	, ref_file_(ref_file)
{
}

void WorkerLowCoverage::run()
{
	try
	{		
		BamReader reader(bam_file_, ref_file_);
		for (int i=chunk_.start; i<=chunk_.end; ++i)
		{
			const BedLine& bed_line = chunk_.data[i];
			const int start = bed_line.start();

			//init coverage statistics
			QVector<int> roi_cov(bed_line.length(), 0);

			//jump to region
			reader.setRegion(bed_line.chr(), start, bed_line.end());

			//iterate through all alignments
			BamAlignment al;
			QBitArray baseQualities;

			while (reader.getNextAlignment(al))
			{
				if (al.isDuplicate()) continue;
				if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
				if (al.isUnmapped() || al.mappingQuality()<min_mapq_) continue;

				const int ol_start = std::max(start, al.start()) - start;
				const int ol_end = std::min(bed_line.end(), al.end()) - start;
				min_baseq_>0 ? countCoverageWithBaseQuality(min_baseq_, roi_cov, start, ol_start, ol_end, baseQualities, al) : countCoverageWithoutBaseQuality(roi_cov, ol_start, ol_end);
			}

			//create low-coverage regions file
			bool reg_open = false;
			int reg_start = -1;
			for (int p=0; p<roi_cov.count(); ++p)
			{
				bool low_cov = roi_cov[p]<cutoff_;
				if (reg_open && !low_cov)
				{
					chunk_.output.append(BedLine(bed_line.chr(), reg_start+start, p+start-1, bed_line.annotations()));
					reg_open = false;
					reg_start = -1;
				}
				if (!reg_open && low_cov)
				{
					reg_open = true;
					reg_start = p;
				}
			}
			if (reg_open)
			{
				chunk_.output.append(BedLine(bed_line.chr(), reg_start+start, bed_line.length()+start-1, bed_line.annotations()));
			}
		}
	}
	catch(Exception& e)
	{
		chunk_.error = e.message();
	}
	catch(std::exception& e)
	{
		chunk_.error = e.what();
	}
	catch(...)
	{
		chunk_.error = "Unknown exception!";
	}
}

void WorkerLowCoverage::countCoverageWithBaseQuality(int min_baseq, QVector<int>& roi_cov, int start, int ol_start, int ol_end, QBitArray& baseQualities, const BamAlignment& al)
{
	int quality_pos = std::max(start, al.start()) - al.start();
	al.qualities(baseQualities, min_baseq, al.end() - al.start() + 1);
	for (int p=ol_start; p<=ol_end; ++p)
	{
		if(baseQualities.testBit(quality_pos))
		{
			++roi_cov[p];
		}
		++quality_pos;
	}
}

void WorkerLowCoverage::countCoverageWithoutBaseQuality(QVector<int>& roi_cov, int ol_start, int ol_end)
{
	for (int p=ol_start; p<=ol_end; ++p)
	{
		++roi_cov[p];
	}
}
