#ifndef THREADCOORDINATOR_H
#define THREADCOORDINATOR_H

#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QTime>
#include "Auxilary.h"

//Coordinator class for analysis, writing, errors. Needed because the "main" function cannot hanle signals/slots.
class ThreadCoordinator
	: public QObject
{
	Q_OBJECT
public:
	ThreadCoordinator(QObject* parent, TrimmingParameters params);
	~ThreadCoordinator();

signals:
	void finished();

private slots:
	//Slot that loads new data into a job
	void load(int i);
	//Slot that analyzes data in a job
	void analyze(int i);
	//Slot that writes a job
	void write(int i);
	//Slot that handles errors
	void error(int i, QString message);
	//Slots that notifies this class if all input data was read
	void inputDone(int i);
	//Slot that checks if the processing is done
	void checkDone();

	//Print status data
	void printStatus();

private:
	InputStreams streams_in_;
	OutputStreams streams_out_;
	QList<AnalysisJob> job_pool_;

	QThreadPool thread_pool_read_;
	QThreadPool thread_pool_analyze_;
	QThreadPool thread_pool_write_;

	TrimmingParameters params_;
	TrimmingStatistics stats_;
	ErrorCorrectionStatistics ec_stats_;

	QTime timer_overall_;
	QTimer timer_progress_;
	QTimer timer_done_;
};

#endif // THREADCOORDINATOR_H
