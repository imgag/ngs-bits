#include "WorkerLowCoverageBed.h"
#include "BamReader.h"
#include "Statistics.h"

WorkerLowCoverageBed::WorkerLowCoverageBed(BedChunk& bed_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file)
	: QRunnable()
	, bed_chunk_(bed_chunk)
	, bam_file_(bam_file)
	, cutoff_(cutoff)
	, min_mapq_(min_mapq)
	, min_baseq_(min_baseq)
	, ref_file_(ref_file)
{
}

void WorkerLowCoverageBed::run()
{
	try
	{				
		BamReader reader(bam_file_, ref_file_);
		for (int i=bed_chunk_.start; i<=bed_chunk_.end; ++i)
		{
			const BedLine& bed_line = bed_chunk_.data[i];
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
				min_baseq_>0 ? Statistics::countCoverageWithBaseQuality(min_baseq_, roi_cov, start, ol_start, ol_end, baseQualities, al) : Statistics::countCoverageWithoutBaseQuality(roi_cov, ol_start, ol_end);
			}

			//create low-coverage regions file
			bool reg_open = false;
			int reg_start = -1;
			for (int p=0; p<roi_cov.count(); ++p)
			{
				bool low_cov = roi_cov[p]<cutoff_;
				if (reg_open && !low_cov)
				{
					bed_chunk_.output.append(BedLine(bed_line.chr(), reg_start+start, p+start-1, bed_line.annotations()));
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
				bed_chunk_.output.append(BedLine(bed_line.chr(), reg_start+start, bed_line.length()+start-1, bed_line.annotations()));
			}
		}
	}
	catch(Exception& e)
	{
		bed_chunk_.error = e.message();
	}
	catch(std::exception& e)
	{
		bed_chunk_.error = e.what();
	}
	catch(...)
	{
		bed_chunk_.error = "Unknown exception!";
	}
}
