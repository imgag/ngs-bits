#ifndef WORKERAVERAGECOVERAGE_H
#define WORKERAVERAGECOVERAGE_H

#include <QRunnable>
#include "BedFile.h"
#include "Exceptions.h"

//Coverage calculation worker using random-access
class WorkerAverageCoverage
	: public QRunnable
{
public:

	struct Chunk
	{
		BedFile& data;
		int start;
		int end;
		QString error; //In case of error

        Chunk(const Chunk& bed_chunk) = default;

		void operator=(const Chunk& rhs)
		{
			if (&data != &rhs.data)
			{
				THROW(NotImplementedException, "Chunk 'data' cannot be reassigned");
			}
			data = rhs.data;
			start = rhs.start;
			end = rhs.end;
			error = rhs.error;
		}
	};

	WorkerAverageCoverage(Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file, bool skip_mismapped, bool debug);
	virtual void run() override;

private:
	Chunk& chunk_;
	QString bam_file_;
	int min_mapq_;
	int decimals_;
	QString ref_file_;
	bool skip_mismapped_;
	bool debug_;
};

//Coverage calculation worker using a chromosome-wise sweep
class WorkerAverageCoverageChr
	: public QRunnable
{
public:

	WorkerAverageCoverageChr(WorkerAverageCoverage::Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file, bool skip_mismapped, bool debug);
	virtual void run() override;

private:
	WorkerAverageCoverage::Chunk& chunk_;
	QString bam_file_;
	int min_mapq_;
	int decimals_;
	QString ref_file_;
	bool skip_mismapped_;
	bool debug_;
};
#endif // WORKERAVERAGECOVERAGE_H
