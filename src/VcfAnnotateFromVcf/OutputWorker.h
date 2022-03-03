#ifndef OUTPUTWORKER_H
#define OUTPUTWORKER_H

#include <QObject>
#include <QRunnable>
#include <QFile>
#include <QSharedPointer>
#include "Auxilary.h"

class OutputWorker
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	OutputWorker(AnalysisJob& job, QSharedPointer<QFile> out_stream, Parameters& params);
	~OutputWorker();
	virtual void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void retry(int i); //signal emitted when a job cannot be written now (order matters)
	void error(int i, QString message); //signal emitted when job failed

protected:
	AnalysisJob& job_;
	QSharedPointer<QFile> out_stream_;
	Parameters& params_;
};

#endif // OUTPUTWORKER_H
