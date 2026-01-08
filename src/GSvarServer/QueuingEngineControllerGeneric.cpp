#include "QueuingEngineControllerGeneric.h"
#include "HttpRequestHandler.h"
#include "ProxyDataService.h"
#include "Log.h"
#include "Settings.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

QueuingEngineControllerGeneric::QueuingEngineControllerGeneric()
{
	proxy_ = QNetworkProxy::NoProxy;
	if(!ProxyDataService::isConnected())
	{
		const QNetworkProxy& proxy = ProxyDataService::getProxy();
		if(proxy.type() != QNetworkProxy::HttpProxy)
		{
			Log::error("No connection to the internet! Please check your proxy settings.");
			return;
		}
		if(proxy.hostName().isEmpty() || (proxy.port() < 1))
		{
			Log::error("HTTP proxy without reqired host name or port provided!");
			return;
		}

		//final check of the connection
		if(!ProxyDataService::isConnected())
		{
			Log::error("No connection to the internet! Please check your proxy settings.");
			return;
		}
	}

	// set proxy for the file download, if needed
	proxy_ = ProxyDataService::getProxy();
	if (proxy_!=QNetworkProxy::NoProxy)
	{
		Log::info("Using Proxy: " + proxy_.hostName());
	}

	qe_api_base_url_ = Settings::string("qe_api_base_url", true);
}

QString QueuingEngineControllerGeneric::getEngineName() const
{
	return "Generic";
}

void QueuingEngineControllerGeneric::submitJob(NGSD &db, int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, int job_id) const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not submit a job: base URL for queuing engine API had not been set");
		return;
	}

	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "submit");
	top_level_json_object.insert("threads", threads);
	top_level_json_object.insert("queues", QJsonArray::fromStringList(queues));
	top_level_json_object.insert("pipeline_args", QJsonArray::fromStringList(pipeline_args));
	top_level_json_object.insert("project_folder", project_folder);
	top_level_json_object.insert("script", script);
	top_level_json_object.insert("job_args", job_args);
	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);
	try
	{
		HttpHeaders add_headers;
		int status_code = HttpRequestHandler(proxy_).post(qe_api_base_url_, json_doc_output.toJson(), add_headers).status_code;
		if (status_code!=200)
		{
			Log::error("There has been an error while submitting a job, response code: " + QString::number(status_code));
		}

	}
	catch(Exception e)
	{
		Log::error("There has been an error while submitting a job: " + e.message());
	}
}

bool QueuingEngineControllerGeneric::updateRunningJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not update a running job: base URL for queuing engine API had not been set");
		return false;
	}

	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "update");


	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);
	try
	{
		HttpHeaders add_headers;
		int status_code = HttpRequestHandler(proxy_).post(qe_api_base_url_, json_doc_output.toJson(), add_headers).status_code;
		if (status_code!=200)
		{
			Log::error("There has been an error while updating a job, response code: " + QString::number(status_code));
		}

	}
	catch(Exception e)
	{
		Log::error("There has been an error while updating a job: " + e.message());
	}
}

void QueuingEngineControllerGeneric::checkCompletedJob(NGSD &db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not check a completed job: base URL for queuing engine API had not been set");
		return;
	}

	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "check");
	top_level_json_object.insert("qe_job_id", qe_job_id);

	QJsonArray json_stdout_stderr;
	for (const QByteArray &list_item : stdout_stderr)
	{
		json_stdout_stderr.append(QString::fromUtf8(list_item));
	}

	top_level_json_object.insert("stdout_stderr", json_stdout_stderr);
	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);
	try
	{
		HttpHeaders add_headers;
		int status_code = HttpRequestHandler(proxy_).post(qe_api_base_url_, json_doc_output.toJson(), add_headers).status_code;
		if (status_code!=200)
		{
			Log::error("There has been an error while checking a completed job, response code: " + QString::number(status_code));
		}

	}
	catch(Exception e)
	{
		Log::error("There has been an error while checking a completed job: " + e.message());
	}
}

void QueuingEngineControllerGeneric::deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not delete a job: base URL for queuing engine API had not been set");
		return;
	}

	QJsonDocument json_doc_output;
	QJsonObject top_level_json_object;
	top_level_json_object.insert("action", "delete");



	top_level_json_object.insert("job_id", job_id);
	json_doc_output.setObject(top_level_json_object);
	try
	{
		HttpHeaders add_headers;
		int status_code = HttpRequestHandler(proxy_).post(qe_api_base_url_, json_doc_output.toJson(), add_headers).status_code;
		if (status_code!=200)
		{
			Log::error("There has been an error while deleting a job, response code: " + QString::number(status_code));
		}

	}
	catch(Exception e)
	{
		Log::error("There has been an error while deleting a job: " + e.message());
	}
}
