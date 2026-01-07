#include "QueuingEngineControllerGeneric.h"
#include "Log.h"

QueuingEngineControllerGeneric::QueuingEngineControllerGeneric()
{

}

QString QueuingEngineControllerGeneric::getEngineName() const
{
	return "Generic";
}

void QueuingEngineControllerGeneric::submitJob(NGSD &db, int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, int job_id) const
{

}

bool QueuingEngineControllerGeneric::updateRunningJob(NGSD &db, const AnalysisJob &job, int job_id) const
{

}

void QueuingEngineControllerGeneric::checkCompletedJob(NGSD &db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{

}

void QueuingEngineControllerGeneric::deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const
{

}
