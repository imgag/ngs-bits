#include "WorkerAverageCoverage.h"
#include "BamReader.h"
#include "ChromosomalIndex.h"

WorkerAverageCoverage::WorkerAverageCoverage(WorkerAverageCoverage::Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file, bool debug)
	: QRunnable()
	, chunk_(chunk)
	, bam_file_(bam_file)
	, min_mapq_(min_mapq)
	, decimals_(decimals)
	, ref_file_(ref_file)
	, debug_(debug)
{
}

void WorkerAverageCoverage::run()
{
	try
	{
		//init
		QTime timer;
		timer.start();

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

		//debug output
		if (debug_) QTextStream(stdout) << "Read processing for chunk with start/end " << chunk_.start << "/" << chunk_.end << " took " << Helper::elapsedTime(timer) << endl;
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

WorkerAverageCoverageChr::WorkerAverageCoverageChr(WorkerAverageCoverage::Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file, bool debug)
	: QRunnable()
	, chunk_(chunk)
	, bam_file_(bam_file)
	, min_mapq_(min_mapq)
	, decimals_(decimals)
	, ref_file_(ref_file)
	, debug_(debug)
{
}

void WorkerAverageCoverageChr::run()
{
	try
	{

		//init
		QTime timer;
		timer.start();

		//open BAM file
		BamReader reader(bam_file_, ref_file_);

		//determine start/end
		Chromosome chr;
		int start = -1;
		int end = -1;
		for (int i=chunk_.start; i<=chunk_.end; ++i)
		{
			const BedLine& bed_line = chunk_.data[i];

			if (!chr.isValid())
			{
				chr = bed_line.chr();
				start = bed_line.start();
				end = bed_line.end();
			}

			if (bed_line.chr()!=chr) THROW(ProgrammingException, "Different chromosomes in WorkerAverageCoverageChr found - this should not happen!");
			if (bed_line.start()<start) start = bed_line.start();
			if (bed_line.end()>end) end = bed_line.end();
		}

		//set region
		reader.setRegion(chr, start, end);

		//create index
		ChromosomalIndex<BedFile> index(chunk_.data);

		//iterate through all alignments
		QHash<int, long> idx2cov;
		BamAlignment al;
		while (reader.getNextAlignment(al))
		{
			if (al.isDuplicate() || al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
			if (al.isUnmapped() || al.mappingQuality()<min_mapq_) continue;

			QVector<int> indices = index.matchingIndices(chr, al.start(), al.end());
			foreach(int i, indices)
			{
				const int ol_start = std::max(chunk_.data[i].start(), al.start());
				const int ol_end = std::min(chunk_.data[i].end(), al.end());
				if (ol_start<=ol_end)
				{
					if (!idx2cov.contains(i)) idx2cov[i] = 0.0;
					idx2cov[i] += (ol_end - ol_start + 1);
				}
			}
		}

		//write back
		for(int i=0; i<chunk_.data.count(); ++i)
		{
			BedLine& bed_line = chunk_.data[i];

			if (bed_line.chr()!=chr) continue;

			bed_line.annotations().append(QByteArray::number((double)idx2cov.value(i, 0) / bed_line.length(), 'f', decimals_));
		}

		//debug output
		if (debug_) QTextStream(stdout) << "Processing chromosome " << chr.str() << " took " << Helper::elapsedTime(timer) << endl;
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
