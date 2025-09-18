#ifndef QUEUINGENGINEEXECUTORPROVIDERSLURM_H
#define QUEUINGENGINEEXECUTORPROVIDERSLURM_H

#include "QueuingEngineExecutorProvider.h"

class QueuingEngineExecutorProviderSlurm
    : virtual public QueuingEngineExecutorProvider
{
public:
    QueuingEngineExecutorProviderSlurm();
    virtual ~QueuingEngineExecutorProviderSlurm() {}

    QString getEngineName() const override;
    QueuingEngineOutput submitJob(int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, QString job_id, bool display_debug) const override;
    QueuingEngineOutput checkJobsForAllUsers() const override;
    QueuingEngineOutput checkJobDetails(QString job_id) const override;
    QueuingEngineOutput checkCompletedJob(QString job_id) const override;
    QueuingEngineOutput deleteJob(QString job_id) const override;
};

#endif // QUEUINGENGINEEXECUTORPROVIDERSLURM_H
