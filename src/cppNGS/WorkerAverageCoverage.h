#ifndef WORKERAVERAGECOVERAGE_H
#define WORKERAVERAGECOVERAGE_H

#include <QRunnable>
#include "BedFile.h"

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

		void operator=(const Chunk& rhs)
		{
			data = rhs.data;
			start = rhs.start;
			end = rhs.end;
			error = rhs.error;
		}
	};

	WorkerAverageCoverage(Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file, bool debug);
	virtual void run() override;

private:
	Chunk& chunk_;
	QString bam_file_;
	int min_mapq_;
	int decimals_;
	QString ref_file_;
	bool debug_;
};

//Coverage calculation worker using a chromosome-wise sweep
class WorkerAverageCoverageChr
	: public QRunnable
{
public:

	WorkerAverageCoverageChr(WorkerAverageCoverage::Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file, bool debug);
	virtual void run() override;

private:
	WorkerAverageCoverage::Chunk& chunk_;
	QString bam_file_;
	int min_mapq_;
	int decimals_;
	QString ref_file_;
	bool debug_;
};
#endif // WORKERAVERAGECOVERAGE_H
