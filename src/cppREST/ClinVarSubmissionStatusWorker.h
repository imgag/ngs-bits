#ifndef CLINVARSUBMISSIONSTATUSWORKER_H
#define CLINVARSUBMISSIONSTATUSWORKER_H

#include "cppREST_global.h"
#include <QRunnable>

// A worker that updates a ClinVar submission status, the
// server runs it on schedule
class CPPRESTSHARED_EXPORT ClinVarSubmissionStatusWorker
	: public QRunnable
{
public:
	explicit ClinVarSubmissionStatusWorker();
	void run() override;
};

#endif // CLINVARSUBMISSIONSTATUSWORKER_H
