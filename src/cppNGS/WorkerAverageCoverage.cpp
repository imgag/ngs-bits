#include "WorkerAverageCoverage.h"
#include "BamReader.h"

WorkerAverageCoverage::WorkerAverageCoverage(WorkerAverageCoverage::Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file)
	: QRunnable()
	, chunk_(chunk)
	, bam_file_(bam_file)
	, min_mapq_(min_mapq)
	, decimals_(decimals)
	, ref_file_(ref_file)
{
}

void WorkerAverageCoverage::run()
{
	try
	{
		//open BAM file
		BamReader reader(bam_file_, ref_file_);

		for (int i=chunk_.start; i<=chunk_.end; ++i)
		{
			BedLine& bed_line = chunk_.data[i];
			long cov = 0;
			//jump to region
			reader.setRegion(bed_line.chr(), bed_line.start(), bed_line.end());

			//iterate through all alignments
			BamAlignment al;
			while (reader.getNextAlignment(al))
			{
				if (al.isDuplicate() || al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
				if (al.isUnmapped() || al.mappingQuality()<min_mapq_) continue;

				const int ol_start = std::max(bed_line.start(), al.start());
				const int ol_end = std::min(bed_line.end(), al.end());
				if (ol_start<=ol_end)
				{
					cov += ol_end - ol_start + 1;
				}
			}
			bed_line.annotations().append(QByteArray::number((double)cov / bed_line.length(), 'f', decimals_));
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
