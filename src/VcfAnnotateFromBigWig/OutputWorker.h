#ifndef OUTPUTWORKER_H
#define OUTPUTWORKER_H

#include <QFile>
#include <QRunnable>
#include <QSharedPointer>
#include "Auxilary.h"


class OutputWorker
		: public QRunnable
{
public:
	OutputWorker(QList<AnalysisJob>& job_pool, QString output_filename);
	void run();
	void terminate()
	{
		terminate_ = true;
	}

protected:
	bool terminate_;
	QList<AnalysisJob>& job_pool_;
	QSharedPointer<QFile> out_p_;
	int write_chunk_;
};

#endif // OUTPUTWORKER_H
