#ifndef QUEUINGENGINEEXECUTORPROVIDERSLURM_H
#define QUEUINGENGINEEXECUTORPROVIDERSLURM_H

#include "QueuingEngineExecutorProvider.h"

class QueuingEngineExecutorProviderSlurm
    : virtual public QueuingEngineExecutorProvider
{
public:
    QueuingEngineExecutorProviderSlurm();
    virtual ~QueuingEngineExecutorProviderSlurm() {}

    int submitJob(QString job_id, QByteArrayList* output = nullptr) const override;
    int checkUserJob(QString job_id, QByteArrayList* output = nullptr) const override;
    int checkJobDetails(QString job_id, QByteArrayList* output = nullptr) const override;
    int checkCompletedJob(QString job_id, QByteArrayList* output = nullptr) const override;
    int deleteJob(QString job_id, QByteArrayList* output = nullptr) const override;
};

#endif // QUEUINGENGINEEXECUTORPROVIDERSLURM_H
