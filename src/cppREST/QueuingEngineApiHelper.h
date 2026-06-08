#ifndef QUEUINGENGINEAPIHELPER_H
#define QUEUINGENGINEAPIHELPER_H

#include "cppREST_global.h"
#include "HttpRequestHandler.h"

class CPPRESTSHARED_EXPORT QueuingEngineApiHelper
{
public:
	QueuingEngineApiHelper(QString api_base_url, QString secure_token);

	ServerReply submitJob(int threads, QStringList queues, QStringList script_args, QString working_directory, QString script) const;
	ServerReply updateRunningJob(QString sge_id, QString sge_queue) const;
	ServerReply deleteJob(QString sge_id, QString type) const;

private:
	QString api_base_url_;
	QString secure_token_;
};

#endif // QUEUINGENGINEAPIHELPER_H
