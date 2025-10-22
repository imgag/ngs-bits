#ifndef QUEUINGENGINECONTROLLER_H
#define QUEUINGENGINECONTROLLER_H

#include "Helper.h"
#include <QTextStream>
#include "PipelineSettings.h"
#include <QRunnable>
#include "NGSD.h"

class QueuingEngineController
		: public QRunnable
{
public:
	// Determines which subclass to use
	static QueuingEngineController* create(const QString& engine);

	void run() override;

protected:
    // Returns the name of a queuing engine
    virtual QString getEngineName() const = 0;

    // Submits a job to a queuing engine and returns an exit code
	virtual void submitJob(NGSD& db, int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, int job_id) const = 0;

	// Updates the status of a running job in NGSD and returns if the job is finished
	virtual bool updateRunningJob(NGSD& db, const AnalysisJob &job, int job_id) const = 0;

    // Performs job accounting after completion and returns an exit code
	virtual void checkCompletedJob(NGSD& db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const = 0;

    // Deletes a job and returns an exit code
	virtual void deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const = 0;

	bool debug_ = false;

private:
	void startAnalysis(NGSD& db, const AnalysisJob& job, int job_id);
	void updateAnalysisStatus(NGSD& db, const AnalysisJob& job, int job_id);

	bool singleSampleAnalysisRunning(NGSD& db, const AnalysisJob& job);
};

#endif // QUEUINGENGINECONTROLLER_H
