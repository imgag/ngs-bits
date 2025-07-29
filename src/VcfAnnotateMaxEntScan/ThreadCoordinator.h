#ifndef THREADCOORDINATOR_H
#define THREADCOORDINATOR_H

#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QFile>
#include <QSharedPointer>
#include "Auxilary.h"

//coordinator class for multi-threading
class ThreadCoordinator
	: public QObject
{
	Q_OBJECT
public:
	ThreadCoordinator(QObject* parent, Parameters params, MetaData meta);
	~ThreadCoordinator();

signals:
	void finished();

public slots:
	void read(int i);
	void annotate(int i);
	void write(int i);
	void error(int i, QString message);

	//slot that notifies this class if all input data was read
	void inputDone(int i);
	//slot that checks if the processing is done
	void checkDone();

private:
	Parameters params_;
	MetaData meta_;
	QSharedPointer<VersatileFile> in_stream_;
	QSharedPointer<QFile> out_stream_;
	QList<AnalysisJob> job_pool_;
	QThreadPool thread_pool_read_;
	QThreadPool thread_pool_annotate_;
	QThreadPool thread_pool_write_;

	bool input_done_ = false;
	QTimer timer_done_;
};

#endif // THREADCOORDINATOR_H

