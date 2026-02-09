#ifndef QUEUINGENGINEAPIHELPER_H
#define QUEUINGENGINEAPIHELPER_H

#include "cppREST_global.h"
#include "Log.h"
#include "Exceptions.h"
#include "Settings.h"
#include "HttpRequestHandler.h"
#include <QNetworkProxy>

class CPPRESTSHARED_EXPORT QueuingEngineApiHelper
{
public:
	QueuingEngineApiHelper(QString api_base_url, QNetworkProxy proxy, QString secure_token);

	ServerReply submitJob(int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, int job_id) const;
	ServerReply updateRunningJob(QString sge_id, QString sge_queue, int job_id) const;
	ServerReply checkCompletedJob(QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const;
	ServerReply deleteJob(QString sge_id, QString type, int job_id) const;

private:
	QString api_base_url_;
	QNetworkProxy proxy_;
	QString secure_token_;
};

#endif // QUEUINGENGINEAPIHELPER_H
