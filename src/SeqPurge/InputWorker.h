#ifndef INPUTWORKER_H
#define INPUTWORKER_H

#include <QRunnable>
#include "Auxilary.h"

class InputWorker
	: public QObject
	, public QRunnable
{
	Q_OBJECT
public:
	InputWorker(AnalysisJob& job, InputStreams& streams, const TrimmingParameters& params);
	~InputWorker();
	virtual void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void error(int i, QString message); //signal emitted when job failed
	void inputDone(int i); //signal emitted when all input data was read

private:
	AnalysisJob& job_;
	InputStreams& streams_;
	const TrimmingParameters& params_;
};

#endif // INPUTWORKER_H
