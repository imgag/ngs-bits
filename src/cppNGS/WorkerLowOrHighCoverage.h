#ifndef WORKERLOWORHIGHCOVERAGE_H
#define WORKERLOWORHIGHCOVERAGE_H

#include <QRunnable>
#include "BedFile.h"
#include "BamReader.h"

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

	WorkerLowOrHighCoverage(Chunk& bed_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high, bool debug);
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
};


class WorkerLowOrHighCoverageChr : public QRunnable
{
public:

	WorkerLowOrHighCoverageChr(WorkerLowOrHighCoverage::Chunk& bed_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high, bool debug);
	virtual void run() override;

private:
	WorkerLowOrHighCoverage::Chunk& chunk_;
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;
	bool is_high_;
	bool debug_;
};


#endif // WORKERLOWORHIGHCOVERAGE_H
