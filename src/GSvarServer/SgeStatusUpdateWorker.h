#ifndef SGESTATUSUPDATEWORKER_H
#define SGESTATUSUPDATEWORKER_H

#include <QRunnable>
#include "NGSD.h"

class SgeStatusUpdateWorker
    : public QRunnable
{

public:
	SgeStatusUpdateWorker();
	void run() override;

private:
	void startAnalysis(NGSD& db, const AnalysisJob& job, int job_id);
	void updateAnalysisStatus(NGSD& db, const AnalysisJob& job, int job_id);
	void canceledAnalysis(NGSD& db, const AnalysisJob& job, int job_id);

	bool singleSampleAnalysisRunning(NGSD& db, const AnalysisJob& job);

	bool debug_ = false;
};

#endif // SGESTATUSUPDATEWORKER_H
