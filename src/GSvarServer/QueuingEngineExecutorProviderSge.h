#ifndef QUEUINGENGINEEXECUTORPROVIDERSGE_H
#define QUEUINGENGINEEXECUTORPROVIDERSGE_H

#include "QueuingEngineExecutorProvider.h"

class QueuingEngineExecutorProviderSge
    : virtual public QueuingEngineExecutorProvider
{
public:
    QueuingEngineExecutorProviderSge();
    virtual ~QueuingEngineExecutorProviderSge() {}

    int submitJob(QString job_id, QByteArrayList* output = nullptr) const override;
    int checkUserJob(QString job_id, QByteArrayList* output = nullptr) const override;
    int checkJobDetails(QString job_id, QByteArrayList* output = nullptr) const override;
    int checkCompletedJob(QString job_id, QByteArrayList* output = nullptr) const override;
    int deleteJob(QString job_id, QByteArrayList* output = nullptr) const override;
};

#endif // QUEUINGENGINEEXECUTORPROVIDERSGE_H
