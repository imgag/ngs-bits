#ifndef ANALYSISWORKER_H
#define ANALYSISWORKER_H

#include <QRunnable>
#include <Auxilary.h>


///Analysis worker
class AnalysisWorker
        : public QRunnable
{
public:
	AnalysisWorker(AnalysisJob& job, TrimmingParameters& params, TrimmingStatistics& stats, ErrorCorrectionStatistics& ecstats);
	void run();

private:
	AnalysisJob& job_;
	const TrimmingParameters& params_;
	TrimmingStatistics& stats_;
	ErrorCorrectionStatistics& ecstats_;

	///Error correction
	void correctErrors(QTextStream& debug_out);
};

#endif


