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

ServerReply QueuingEngineApiHelper::submitJob(int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, int job_id) const
{
	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "submit");
	top_level_json_object.insert("token", secure_token_);
	top_level_json_object.insert("threads", threads);
	top_level_json_object.insert("queues", QJsonArray::fromStringList(queues));
	top_level_json_object.insert("pipeline_args", QJsonArray::fromStringList(pipeline_args));
	top_level_json_object.insert("project_folder", project_folder);
	top_level_json_object.insert("script", script);
	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);

	HttpHeaders add_headers;
	add_headers.insert("Content-Type", "application/json");
	return HttpRequestHandler(proxy_).post(api_base_url_, json_doc_output.toJson(), add_headers);
}

ServerReply QueuingEngineApiHelper::updateRunningJob(QString sge_id, QString sge_queue, int job_id) const
{
	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "update");
	top_level_json_object.insert("token", secure_token_);
	top_level_json_object.insert("qe_job_id", sge_id);
	top_level_json_object.insert("qe_job_queue", sge_queue);
	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);

	HttpHeaders add_headers;
	add_headers.insert("Content-Type", "application/json");
	return HttpRequestHandler(proxy_).post(api_base_url_, json_doc_output.toJson(), add_headers);
}

ServerReply QueuingEngineApiHelper::checkCompletedJob(QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{
	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "check");
	top_level_json_object.insert("token", secure_token_);
	top_level_json_object.insert("qe_job_id", qe_job_id);

	QJsonArray json_stdout_stderr;
	for (const QByteArray &list_item : stdout_stderr)
	{
		json_stdout_stderr.append(QString::fromUtf8(list_item));
	}

	top_level_json_object.insert("stdout_stderr", json_stdout_stderr);
	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);

	HttpHeaders add_headers;
	add_headers.insert("Content-Type", "application/json");
	return HttpRequestHandler(proxy_).post(api_base_url_, json_doc_output.toJson(), add_headers);
}

ServerReply QueuingEngineApiHelper::deleteJob(QString sge_id, QString type, int job_id) const
{
	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "delete");
	top_level_json_object.insert("token", secure_token_);
	top_level_json_object.insert("qe_job_id", sge_id);
	top_level_json_object.insert("qe_job_type", type);
	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);

	HttpHeaders add_headers;
	add_headers.insert("Content-Type", "application/json");
	return HttpRequestHandler(proxy_).post(api_base_url_, json_doc_output.toJson(), add_headers);
}
