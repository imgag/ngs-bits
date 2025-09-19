#ifndef QUEUINGENGINEEXECUTORPROVIDER_H
#define QUEUINGENGINEEXECUTORPROVIDER_H

#include "Helper.h"

class QueuingEngineExecutorProvider
{
public:
    // Submits a job to a queuing engine and returns an exit code
    virtual int submitJob(QString job_id, QByteArrayList* output = nullptr) const = 0;

    // Checks the status of a job from a specific user and returns an exit code
    virtual int checkUserJob(QString job_id, QByteArrayList* output = nullptr) const = 0;

    // Checks the status of a job, creates detailed info, and returns an exit code
    virtual int checkJobDetails(QString job_id, QByteArrayList* output = nullptr) const = 0;

    // Performs job accounting after completion and returns an exit code
    virtual int checkCompletedJob(QString job_id, QByteArrayList* output = nullptr) const = 0;

    // Deletes a job and returns an exit code
    virtual int deleteJob(QString job_id, QByteArrayList* output = nullptr) const = 0;
};

#endif // QUEUINGENGINEEXECUTORPROVIDER_H
