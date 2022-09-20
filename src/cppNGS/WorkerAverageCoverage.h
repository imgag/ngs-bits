#ifndef WORKERAVERAGECOVERAGE_H
#define WORKERAVERAGECOVERAGE_H

#include <QRunnable>
#include "BedFile.h"

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

		Chunk& operator=(const Chunk& /*item*/)
		{
			return *this;
		}
	};

	WorkerAverageCoverage(Chunk& chunk, QString bam_file, int min_mapq, int decimals, QString ref_file);
	virtual void run() override;

private:
	Chunk& chunk_;
	QString bam_file_;
	int min_mapq_;
	int decimals_;
	QString ref_file_;
};

#endif // WORKERAVERAGECOVERAGE_H
