#include "WorkerLowOrHighCoverage.h"
#include "BamReader.h"
#include "Statistics.h"

WorkerLowOrHighCoverage::WorkerLowOrHighCoverage(Chunk& bed_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high, bool debug)
	: QRunnable()
	, chunk_(bed_chunk)
	, bam_file_(bam_file)
	, cutoff_(cutoff)
	, min_mapq_(min_mapq)
	, min_baseq_(min_baseq)
	, ref_file_(ref_file)
	, is_high_(is_high)
	, debug_(debug)
{
}

void WorkerLowOrHighCoverage::run()
{
	try
	{
        QElapsedTimer timer;
		timer.start();
        if (debug_) QTextStream(stdout) << "Processing chunk (" << chunk_.start << "-" << chunk_.end << ")" << QT_ENDL;

		BamReader reader(bam_file_, ref_file_);
		for (int i=chunk_.start; i<=chunk_.end; ++i)
		{
			const BedLine& bed_line = chunk_.data[i];
			int start = bed_line.start();

			//init coverage statistics
			QVector<int> roi_cov(bed_line.length(), 0);

			//jump to region
			reader.setRegion(bed_line.chr(), start, bed_line.end());

			//iterate through all alignments
			BamAlignment al;
			QBitArray base_qualities;
			while (reader.getNextAlignment(al))
			{
				if (al.isDuplicate()) continue;
				if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
				if (al.isUnmapped() || al.mappingQuality()<min_mapq_) continue;

				const int ol_start = std::max(start, al.start()) - start;
				const int ol_end = std::min(bed_line.end(), al.end()) - start;
				if (min_baseq_>0)
				{
					int quality_pos = std::max(start, al.start()) - al.start();
					al.qualities(base_qualities, min_baseq_, al.end() - al.start() + 1);
					for (int p=ol_start; p<=ol_end; ++p)
					{
						if(base_qualities.testBit(quality_pos))
						{
							++roi_cov[p];
						}
						++quality_pos;
					}
				}
				else
				{
					for (int p=ol_start; p<=ol_end; ++p)
					{
						++roi_cov[p];
					}
				}
			}

			//create low-coverage regions file
			bool reg_open = false;
			int reg_start = -1;
			for (int p=0; p<roi_cov.count(); ++p)
			{
				bool filter;
				if (is_high_)
				{
					// high coverage
					filter = roi_cov[p]>=cutoff_;
				}
				else
				{
					// low coverage
					filter = roi_cov[p]<cutoff_;
				}

				if (reg_open && !filter)
				{
					chunk_.output.append(BedLine(bed_line.chr(), reg_start+start, p+start-1, bed_line.annotations()));
					reg_open = false;
					reg_start = -1;
				}
				if (!reg_open && filter)
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

		//debug output
        if (debug_) QTextStream(stdout) << "Processing chunk (" << chunk_.start << "-" << chunk_.end << ") took " << Helper::elapsedTime(timer) << QT_ENDL;
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

WorkerLowOrHighCoverageChr::WorkerLowOrHighCoverageChr(WorkerLowOrHighCoverage::Chunk& bed_chunk, const ChromosomalIndex<BedFile>& bed_index, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high, bool debug)
	: QRunnable()
	, chunk_(bed_chunk)
	, bed_index_(bed_index)
	, bam_file_(bam_file)
	, cutoff_(cutoff)
	, min_mapq_(min_mapq)
	, min_baseq_(min_baseq)
	, ref_file_(ref_file)
	, is_high_(is_high)
	, debug_(debug)
{
}

void WorkerLowOrHighCoverageChr::run()
{
	try
	{
		//sanity checks
		if (chunk_.start<0) THROW(ArgumentException, "Chunk start index is less than zero!");
		if (chunk_.start>chunk_.end) THROW(ArgumentException, "Chunk start index is after chunk end index!");
		if (chunk_.end>=chunk_.data.count()) THROW(ArgumentException, "Chunk end index is behind data end!");
		if (cutoff_>255) THROW(ArgumentException, "Cutoff cannot be bigger than 255!");

		//init
		Chromosome chr = chunk_.data[chunk_.start].chr();
        if (debug_) QTextStream(stdout) << "Sarting processing chromosome " << chr.str() << QT_ENDL;
        QElapsedTimer timer;
		timer.start();

		//open BAM file
        if (debug_) QTextStream(stdout) << "Opening BAM reader for " << chr.str() << QT_ENDL;
		BamReader reader(bam_file_, ref_file_);

		//fill coverage array
        if (debug_) QTextStream(stdout) << "Determining chromosome size for " << chr.str() << QT_ENDL;
		int max_pos = reader.chromosomeSize(chr);
        if (debug_) QTextStream(stdout) << "creating coverage array for " << chr.str() << QT_ENDL;
		QVector<unsigned char> cov;
		cov.fill(0, max_pos+1);

		//iterate through all alignments
        if (debug_) QTextStream(stdout) << "Processing chromosome " << chr.str() << " - max position=" << max_pos << " start index=" << chunk_.start << " end index=" << chunk_.end << QT_ENDL;
		BamAlignment al;
		QBitArray base_qualities;
		reader.setRegion(chr, 0, max_pos);
		while (reader.getNextAlignment(al))
		{
			if (al.isDuplicate()) continue;
			if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
			if (al.isUnmapped() || al.mappingQuality()<min_mapq_) continue;

			//check if read overlaps with BED file
			int start = al.start();
			int end = al.end();
			if (bed_index_.matchingIndex(chr, start, end)==-1) continue;

			if (min_baseq_>0)
			{
				al.qualities(base_qualities, min_baseq_, end - start + 1);
				int quality_pos = 0;
				for (int p=start; p<=end; ++p)
				{
					if(base_qualities.testBit(quality_pos))
					{
						if (cov[p]<254) ++cov[p];
					}
					++quality_pos;
				}
			}
			else
			{
				for (int p=start; p<=end; ++p)
				{
					if (cov[p]<254) ++cov[p];
				}
			}
		}

		//debug output
        if (debug_) QTextStream(stdout) << "Creating output for chromosome " << chr.str() << QT_ENDL;

		//create low-coverage regions for processed chunk
		for (int i=chunk_.start; i<=chunk_.end; ++i)
		{
			const BedLine& bed_line = chunk_.data[i];
			bool reg_open = false;
			int reg_start = -1;
			for (int p=bed_line.start(); p<=bed_line.end(); ++p)
			{
				bool filter = is_high_ ? (cov[p]>=cutoff_) : (cov[p]<cutoff_);

				if (reg_open && !filter)
				{
					chunk_.output.append(BedLine(chr, reg_start, p-1, bed_line.annotations()));
					reg_open = false;
					reg_start = -1;
				}
				if (!reg_open && filter)
				{
					reg_open = true;
					reg_start = p;
				}
			}
			if (reg_open)
			{
				chunk_.output.append(BedLine(chr, reg_start, bed_line.end(), bed_line.annotations()));
			}
		}

		//debug output
        if (debug_) QTextStream(stdout) << "Processing chromosome " << chr.str() << " took " << Helper::elapsedTime(timer) << QT_ENDL;
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
