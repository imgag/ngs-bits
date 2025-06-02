#ifndef WORKERLOWORHIGHCOVERAGE_H
#define WORKERLOWORHIGHCOVERAGE_H

#include <QRunnable>
#include "BedFile.h"
#include "BamReader.h"
#include "ChromosomalIndex.h"

class WorkerLowOrHighCoverage : public QRunnable
{
public:
	struct Chunk
    {
        const BedFile& data;
		int start;
		int end;
		QString error; //In case of error
		BedFile output;

        Chunk(const Chunk& bed_chunk) = default;

        void operator=(const Chunk& bed_chunk)
		{
			if (&data != &bed_chunk.data)
			{
				THROW(NotImplementedException, "Chunk 'data' cannot be reassigned");
			}
			start = bed_chunk.start;
			end = bed_chunk.end;
			error = bed_chunk.error;
			output = bed_chunk.output;
		}
	};

	WorkerLowOrHighCoverage(Chunk& bed_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high, bool debug, bool high_depth);
	virtual void run() override;

private:
	Chunk& chunk_;
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;
	bool is_high_;
	bool debug_;
	bool high_depth_;
};


class WorkerLowOrHighCoverageChr : public QRunnable
{
public:

	WorkerLowOrHighCoverageChr(WorkerLowOrHighCoverage::Chunk& bed_chunk, const ChromosomalIndex<BedFile>& bed_index, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high, bool debug, bool high_depth);
	virtual void run() override;

private:
	WorkerLowOrHighCoverage::Chunk& chunk_;
	const ChromosomalIndex<BedFile>& bed_index_;
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;
	bool is_high_;
	bool debug_;
	bool high_depth_;
};


#endif // WORKERLOWORHIGHCOVERAGE_H
