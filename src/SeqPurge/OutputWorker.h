#ifndef OUTPUTWORKER_H
#define OUTPUTWORKER_H

#include <QRunnable>
#include "Auxilary.h"

//Output worker that makes sure that only one job is written at a time (writing to the two main output FASTQs is done in separate threads to double the maximum possible throughput)
class OutputWorker
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	OutputWorker(AnalysisJob& job, OutputStreams& streams, const TrimmingParameters& params, TrimmingStatistics& stats);
	virtual ~OutputWorker();
	virtual void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void error(int i, QString message); //signal emitted when job failed

private:
	AnalysisJob& job_;
	OutputStreams& streams_;
	const TrimmingParameters& params_;
	TrimmingStatistics& stats_;
};

#endif // OUTPUTWORKER_H


