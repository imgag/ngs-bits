#ifndef QUEUINGENGINEEXECUTORPROVIDER_H
#define QUEUINGENGINEEXECUTORPROVIDER_H

#include "Helper.h"
#include "PipelineSettings.h"
#include "QueuingEngineOutput.h"

class QueuingEngineExecutorProvider
{
public:
    // Returns the name of a queuing engine
    virtual QString getEngineName() const = 0;

    // Submits a job to a queuing engine and returns an exit code
    virtual QueuingEngineOutput submitJob(int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, QString job_id, bool display_debug) const = 0;

    // Checks the status of all jobs for all users and returns an exit code
    virtual QueuingEngineOutput checkJobsForAllUsers() const = 0;

    // Checks the status of a job, creates detailed info, and returns an exit code
    virtual QueuingEngineOutput checkJobDetails(QString job_id) const = 0;

    // Performs job accounting after completion and returns an exit code
    virtual QueuingEngineOutput checkCompletedJob(QString job_id) const = 0;

    // Deletes a job and returns an exit code
    virtual QueuingEngineOutput deleteJob(QString job_id) const = 0;
};

#endif // QUEUINGENGINEEXECUTORPROVIDER_H
