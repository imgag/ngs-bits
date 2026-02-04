#ifndef QUEUINGENGINEAPIHELPER_H
#define QUEUINGENGINEAPIHELPER_H

#include "Log.h"
#include "Exceptions.h"
#include "Settings.h"
#include "cppREST_global.h"
#include <QNetworkProxy>

class CPPRESTSHARED_EXPORT QueuingEngineApiHelper
{
public:
	QueuingEngineApiHelper(QString api_base_url, QNetworkProxy proxy);

	int submitJob(int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, int job_id) const;
	int updateRunningJob(QString sge_id, QString sge_queue, int job_id) const;
	int checkCompletedJob(QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const;
	int deleteJob(QString sge_id, QString type, int job_id) const;

private:
	QString api_base_url_;
	QNetworkProxy proxy_;
};

#endif // QUEUINGENGINEAPIHELPER_H
