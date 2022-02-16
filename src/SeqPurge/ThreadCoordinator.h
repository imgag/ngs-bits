#ifndef THREADCOORDINATOR_H
#define THREADCOORDINATOR_H

#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include "Auxilary.h"

//Coordinator class for analysis, writing, errors. Needed because the "main" function cannot hanle signals/slots.
class ThreadCoordinator
	: public QObject
{
	Q_OBJECT
public:
	ThreadCoordinator(QObject* parent, QStringList in1_files, QStringList in2_files, OutputStreams streams, TrimmingParameters params);
	~ThreadCoordinator();

signals:
	void finished();

private slots:
	//Slot that writes a job
	void write(int i);
	//Slot that load new data into a job
	void load(int i);
	//Slot that handles errors
	void error(int i, QString message);
	//Slot that checks if the processing is done
	void checkDone();

	//Print status data
	void printStatus();

private:
	QStringList in1_files_;
	QStringList in2_files_;
	OutputStreams streams_;
	QList<AnalysisJob> job_pool_;
	TrimmingParameters params_;
	TrimmingStatistics stats_;
	ErrorCorrectionStatistics ec_stats_;
	QTimer timer_progress_;
	QTimer timer_done_;

	//Reads a input read pair. Returns true, if read. Returns false if no more data can be read.
	bool readPair(AnalysisJob& job);
};

#endif // THREADCOORDINATOR_H
