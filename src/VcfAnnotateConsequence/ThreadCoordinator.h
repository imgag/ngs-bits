#ifndef THREADCOORDINATOR_H
#define THREADCOORDINATOR_H

#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QSharedPointer>
#include <QMutex>
#include <zlib.h>
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

	void update_stats(int annotated, int skipped);
	//slot that notifies this class if all input data was read
	void inputDone(int i);
	//slot that checks if the processing is done
	void checkDone();

private:
	Parameters params_;
	MetaData meta_;
	gzFile in_stream_;
	QSharedPointer<QFile> out_stream_;
	QList<AnalysisJob> job_pool_;
	QThreadPool thread_pool_read_;
	QThreadPool thread_pool_annotate_;
	QThreadPool thread_pool_write_;


	QMutex c_mutex;
	int c_annotated_ = 0;
	int c_skipped_ = 0;

	bool input_done_ = false;
	QTimer timer_done_;
    QElapsedTimer timer_annotation_;
};

#endif // THREADCOORDINATOR_H

