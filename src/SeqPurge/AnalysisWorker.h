#ifndef ANALYSISWORKER_H
#define ANALYSISWORKER_H

#include <QRunnable>
#include "Auxilary.h"

///Analysis worker
class AnalysisWorker
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	AnalysisWorker(AnalysisJob& job, TrimmingParameters& params, TrimmingStatistics& stats, ErrorCorrectionStatistics& ecstats);
	virtual ~AnalysisWorker();
	virtual void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void error(int i, QString message); //signal emitted when job failed

private:
	AnalysisJob& job_;
	const TrimmingParameters& params_;
	TrimmingStatistics& stats_;
	ErrorCorrectionStatistics& ecstats_;

	///Error correction
	void correctErrors(int r, QTextStream& debug_out);
};

#endif


