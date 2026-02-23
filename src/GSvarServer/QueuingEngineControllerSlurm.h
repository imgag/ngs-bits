#ifndef QUEUINGENGINECONTROLLERSLURM_H
#define QUEUINGENGINECONTROLLERSLURM_H

#include "QueuingEngineController.h"

class QueuingEngineControllerSlurm
	: public QueuingEngineController
{
public:
	QueuingEngineControllerSlurm();

protected:
	QString getEngineName() const override;
	void submitJob(NGSD& db, int threads, QStringList queues, QStringList pipeline_args, QString working_directory, QString script, int job_id) const override;
	bool updateRunningJob(NGSD& db, const AnalysisJob &job, int job_id) const override;
	void checkCompletedJob(NGSD& db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const override;
	void deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const override;
};

#endif // QUEUINGENGINECONTROLLERSLURM_H
