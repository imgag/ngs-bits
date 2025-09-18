#ifndef QUEUINGENGLINESUPDATEWORKER_H
#define QUEUINGENGLINESUPDATEWORKER_H

#include <QRunnable>
#include "NGSD.h"
#include "QueuingEngineExecutorProvider.h"

class QueuingEngineStatusUpdateWorker
    : public QRunnable
{

public:
    QueuingEngineStatusUpdateWorker();
	void run() override;

private:
	void startAnalysis(NGSD& db, const AnalysisJob& job, int job_id);
	void updateAnalysisStatus(NGSD& db, const AnalysisJob& job, int job_id);
	void canceledAnalysis(NGSD& db, const AnalysisJob& job, int job_id);    

	bool singleSampleAnalysisRunning(NGSD& db, const AnalysisJob& job);

	bool debug_ = false;
    QSharedPointer<QueuingEngineExecutorProvider> executor_provider_;
};

#endif // QUEUINGENGLINESUPDATEWORKER_H
