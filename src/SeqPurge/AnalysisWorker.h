#ifndef ANALYSISWORKER_H
#define ANALYSISWORKER_H

#include <Auxilary.h>

///Analysis worker
class AnalysisWorker
		: public QObject
{
	Q_OBJECT

public:
	AnalysisWorker(AnalysisJob& job, TrimmingParameters& params, TrimmingStatistics& stats, ErrorCorrectionStatistics& ecstats);
	void run();

signals:
	void write(AnalysisJob* job);

private:
	AnalysisJob& job_;
	const TrimmingParameters& params_;
	TrimmingStatistics& stats_;
	ErrorCorrectionStatistics& ecstats_;

	///Error correction
	void correctErrors(QTextStream& debug_out);
};

#endif


