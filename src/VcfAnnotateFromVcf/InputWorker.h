#ifndef INPUTWORKER_H
#define INPUTWORKER_H

#include <QObject>
#include <QRunnable>
#include <zlib.h>
#include "Auxilary.h"

class InputWorker
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	InputWorker(AnalysisJob& job, gzFile& in_stream, Parameters& params);
	virtual void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void error(int i, QString message); //signal emitted when job failed
	void inputDone(int i); //signal emitted when all input data was read

private:
	AnalysisJob& job_;
	gzFile& in_stream_;
	Parameters& params_;
};

#endif // INPUTWORKER_H
