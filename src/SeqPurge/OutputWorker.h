#ifndef OUTPUTWORKER_H
#define OUTPUTWORKER_H

#include <Auxilary.h>

class OutputWorker
	: public QObject
{
	Q_OBJECT

public:
	OutputWorker(QString out1, QString out2, QString out3_base, const TrimmingParameters& params, TrimmingStatistics& stats);

public slots:
	void write(AnalysisJob* job);
	void threadStarted();
	void threadFinished();

protected:
	QTextStream out_;
	QSharedPointer<FastqOutfileStream> ostream1;
	QSharedPointer<FastqOutfileStream> ostream2;
	QSharedPointer<FastqOutfileStream> ostream3;
	QSharedPointer<FastqOutfileStream> ostream4;
	const TrimmingParameters& params_;
	TrimmingStatistics& stats_;
};

#endif // OUTPUTWORKER_H


