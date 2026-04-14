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

	ServerReply submitJob(int threads, QStringList queues, QStringList script_args, QString working_directory, QString script) const;
	ServerReply updateRunningJob(QString sge_id, QString sge_queue) const;
	ServerReply deleteJob(QString sge_id, QString type) const;

private:
	QString api_base_url_;
	QNetworkProxy proxy_;
	QString secure_token_;
};

#endif // QUEUINGENGINEAPIHELPER_H
