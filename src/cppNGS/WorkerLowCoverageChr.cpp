#include "WorkerLowCoverageChr.h"

WorkerLowCoverageChr::WorkerLowCoverageChr(ChrChunk& chr_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file)
	: QRunnable()
	, chr_chunk_(chr_chunk)
	, bam_file_(bam_file)
	, cutoff_(cutoff)
	, min_mapq_(min_mapq)
	, min_baseq_(min_baseq)
	, ref_file_(ref_file)
{
}


void WorkerLowCoverageChr::run()
{
	try
	{
		if (cutoff_>255) THROW(ArgumentException, "Cutoff cannot be bigger than 255!");

		//open BAM file
		BamReader reader(bam_file_, ref_file_);
		QVector<unsigned char> cov;

		if (!chr_chunk_.chr.isNonSpecial()) return;

		int chr_size = chr_chunk_.end;
		cov.fill(0, chr_size);

		//jump to chromosome
		reader.setRegion(chr_chunk_.chr, 0, chr_size);

		//iterate through all alignments
		BamAlignment al;
		QBitArray baseQualities;

		while (reader.getNextAlignment(al))
		{
			if (al.isDuplicate()) continue;
			if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
			if (al.isUnmapped() || al.mappingQuality()<min_mapq_) continue;

			min_baseq_>0 ? StatHelper::countCoverageWGSWithBaseQuality(min_baseq_, cov, al.start() - 1, al.end(), baseQualities, al) : StatHelper::countCoverageWGSWithoutBaseQuality(al.start()-1, al.end(), cov);

		}

		//create low-coverage regions file
		bool reg_open = false;
		int reg_start = -1;
		for (int p=0; p<chr_size; ++p)
		{
			bool low_cov = cov[p]<cutoff_;
			if (reg_open && !low_cov)
			{
				chr_chunk_.output.append(BedLine(chr_chunk_.chr, reg_start+1, p));
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
			chr_chunk_.output.append(BedLine(chr_chunk_.chr, reg_start+1, chr_size));
		}

	}
	catch(Exception& e)
	{
		chr_chunk_.error = e.message();
	}
	catch(std::exception& e)
	{
		chr_chunk_.error = e.what();
	}
	catch(...)
	{
		chr_chunk_.error = "Unknown exception!";
	}
}
