#include "QueuingEngineApiHelper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QueuingEngineApiHelper::QueuingEngineApiHelper(QString api_base_url, QNetworkProxy proxy, QString secure_token)
	: api_base_url_(api_base_url)
	, proxy_(proxy)
	, secure_token_(secure_token)
{
}

ServerReply QueuingEngineApiHelper::submitJob(int threads, QStringList queues, QStringList script_args, QString working_directory, QString script) const
{
	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "submit");
	top_level_json_object.insert("token", secure_token_);
	top_level_json_object.insert("threads", threads);
	top_level_json_object.insert("queues", QJsonArray::fromStringList(queues));
	top_level_json_object.insert("script_args", QJsonArray::fromStringList(script_args));
	top_level_json_object.insert("working_directory", working_directory);
	top_level_json_object.insert("script", script);	
	json_doc_output.setObject(top_level_json_object);

	HttpHeaders add_headers;
	add_headers.insert("Content-Type", "application/json");
	return HttpRequestHandler(proxy_).post(api_base_url_, json_doc_output.toJson(), add_headers);
}

ServerReply QueuingEngineApiHelper::updateRunningJob(QString sge_id, QString sge_queue) const
{
	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "update");
	top_level_json_object.insert("token", secure_token_);
	top_level_json_object.insert("qe_job_id", sge_id);
	top_level_json_object.insert("qe_job_queue", sge_queue);	
	json_doc_output.setObject(top_level_json_object);

	HttpHeaders add_headers;
	add_headers.insert("Content-Type", "application/json");
	return HttpRequestHandler(proxy_).post(api_base_url_, json_doc_output.toJson(), add_headers);
}

ServerReply QueuingEngineApiHelper::deleteJob(QString sge_id, QString type) const
{
	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "delete");
	top_level_json_object.insert("token", secure_token_);
	top_level_json_object.insert("qe_job_id", sge_id);
	top_level_json_object.insert("qe_job_type", type);	
	json_doc_output.setObject(top_level_json_object);

	HttpHeaders add_headers;
	add_headers.insert("Content-Type", "application/json");
	return HttpRequestHandler(proxy_).post(api_base_url_, json_doc_output.toJson(), add_headers);
}
