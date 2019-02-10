#ifndef OUTPUTWORKER_H
#define OUTPUTWORKER_H

#include <QRunnable>
#include <Auxilary.h>

class OutputWorker
	: public QRunnable
{
public:
	OutputWorker(QList<AnalysisJob>& job_pool, QString out1, QString out2, QString out3_base, const TrimmingParameters& params, TrimmingStatistics& stats);
	void run();
	void terminate()
	{
		terminate_ = true;
	}

protected:
	bool terminate_;
	QList<AnalysisJob>& job_pool_;
	QSharedPointer<FastqOutfileStream> ostream1;
	QSharedPointer<FastqOutfileStream> ostream2;
	QSharedPointer<FastqOutfileStream> ostream3;
	QSharedPointer<FastqOutfileStream> ostream4;
	const TrimmingParameters& params_;
	TrimmingStatistics& stats_;
};

#endif // OUTPUTWORKER_H


