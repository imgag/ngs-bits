#include "QueuingEngineControllerGeneric.h"
#include "HttpRequestHandler.h"
#include "ProxyDataService.h"
#include "Log.h"
#include "Settings.h"

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
	secure_token_ = Settings::string("qe_secure_token", true);
}

QString QueuingEngineControllerGeneric::getEngineName() const
{
	return "Generic";
}

void QueuingEngineControllerGeneric::submitJob(NGSD &db, int threads, QStringList queues, QStringList pipeline_args, QString working_directory, QString script, int job_id) const
{
	if (!hasApiUrl()) return;

	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).submitJob(threads, queues, pipeline_args, working_directory, script);
		if (reply.status_code!=200)
		{
			Log::error("There has been an error while submitting a job, response code: " + QString::number(reply.status_code));
		}

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		if (!passedInitialCheck(reply_doc, job_id)) return;

		bool ok = false;
		int exit_code = getCommandExitCode(reply_doc, ok);
		if (!ok) return;
		QString qe_job_id = getJobId(reply_doc, ok);
		if (exit_code!=0 || qe_job_id.isEmpty())
		{
			QByteArrayList results = getResults(reply_doc);
			db.addAnalysisHistoryEntry(job_id, "error", results);
			return;
		}

		db.getQuery().exec("UPDATE analysis_job SET sge_id='" + qe_job_id + "' WHERE id="+QString::number(job_id));
		db.addAnalysisHistoryEntry(job_id, "started", QByteArrayList());

	}
	catch(Exception e)
	{
		Log::error("There has been an error while submitting a job: " + e.message());
	}
}

bool QueuingEngineControllerGeneric::updateRunningJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	bool job_finished = true;
	if (!hasApiUrl()) return job_finished;

	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).updateRunningJob(job.sge_id, job.sge_queue);
		if (reply.status_code!=200)
		{
			Log::error("There has been an error while updating a job, response code: " + QString::number(reply.status_code));
		}

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		if (!passedInitialCheck(reply_doc, job_id)) return job_finished;

		bool ok = false;
		int exit_code = getCommandExitCode(reply_doc, ok);
		if (!ok) return job_finished;

		QString status = getStatus(reply_doc, ok);
		if (!ok) return job_finished;

		if (exit_code==0 && status=="queued/running")
		{
			job_finished = false;
			QString queue = getQueue(reply_doc, ok);
			if (!ok) return job_finished;

			SqlQuery query = db.getQuery();
			query.prepare("UPDATE analysis_job SET sge_queue=:0 WHERE id=:1");
			query.bindValue(0, queue);
			query.bindValue(1, job_id);
			query.exec();
		}
	}
	catch(Exception e)

	{
		Log::error("There has been an error while updating a job: " + e.message());
	}

	return job_finished;
}

void QueuingEngineControllerGeneric::checkCompletedJob(NGSD &db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{
	if (!hasApiUrl()) return;

	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).checkCompletedJob(qe_job_id, stdout_stderr);
		if (reply.status_code!=200)
		{
			Log::error("There has been an error while checking a completed job, response code: " + QString::number(reply.status_code));
		}

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		if (!passedInitialCheck(reply_doc, job_id)) return;

		bool ok = false;
		int exit_code = getCommandExitCode(reply_doc, ok);
		if (!ok) return;

		if (exit_code == 0)
		{
			int qe_exit_code = getEngineExitCode(reply_doc, ok);
			if (!ok) return;

			if (qe_exit_code == 0)
			{
				db.addAnalysisHistoryEntry(job_id, "finished", stdout_stderr);
			}
			else
			{
				stdout_stderr.prepend(("job exit code: " + QString::number(qe_exit_code)).toLatin1());
				db.addAnalysisHistoryEntry(job_id, "error", stdout_stderr);
			}
		}

	}
	catch(Exception e)
	{
		Log::error("There has been an error while checking a completed job: " + e.message());
	}
}

void QueuingEngineControllerGeneric::deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	if (!hasApiUrl()) return;

	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).deleteJob(job.sge_id, job.type);
		if (reply.status_code!=200)
		{
			Log::error("There has been an error while deleting a job, response code: " + QString::number(reply.status_code));

		}

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		if (!passedInitialCheck(reply_doc, job_id)) return;

		QByteArrayList results = getResults(reply_doc);
		db.addAnalysisHistoryEntry(job_id, "canceled", results);
	}
	catch(Exception e)
	{
		Log::error("There has been an error while deleting a job: " + e.message());
	}
}

bool QueuingEngineControllerGeneric::hasApiUrl() const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not delete a job: base URL for queuing engine API had not been set");
		return false;
	}
	return true;
}

bool QueuingEngineControllerGeneric::passedInitialCheck(QJsonDocument &reply_doc, int job_id) const
{
	if (!reply_doc.isObject())
	{
		Log::warn("The queuing engine API replied with an invalid JSON object for the job '" + QString::number(job_id)+ "'");
		return false;
	}
	QJsonObject reply_obj = reply_doc.object();
	if (!reply_obj.contains("result"))
	{
		Log::warn("The queuing engine API reply for the job '" + QString::number(job_id)+ "' does not contain a message field");
		return false;
	}
	return true;
}

QByteArrayList QueuingEngineControllerGeneric::getResults(QJsonDocument &reply_doc) const
{
	QJsonObject reply_obj = reply_doc.object();
	QString result = reply_obj.value("result").toString();
	QByteArrayList results_as_list;
	for (const QString &line : result.split('\n'))
	{
		results_as_list << line.toUtf8();
	}
	return results_as_list;
}

QString QueuingEngineControllerGeneric::getJobId(QJsonDocument &reply_doc, bool &ok) const
{
	QJsonObject reply_obj = reply_doc.object();
	if (!reply_obj.contains("qe_job_id"))
	{
		Log::warn("The queuing engine API reply does not contain a queuing engine job id");
		ok = false;
		return "";
	}
	ok = true;
	return reply_obj.value("qe_job_id").toString();
}

QString QueuingEngineControllerGeneric::getStatus(QJsonDocument &reply_doc, bool &ok) const
{
	QJsonObject reply_obj = reply_doc.object();
	if (!reply_obj.contains("status"))
	{
		Log::warn("The queuing engine API reply does not contain the status");
		ok = false;
		return "";
	}
	ok = true;
	return reply_obj.value("status").toString();
}

int QueuingEngineControllerGeneric::getCommandExitCode(QJsonDocument &reply_doc, bool &ok) const
{
	QJsonObject reply_obj = reply_doc.object();
	if (!reply_obj.contains("cmd_exit_code"))
	{
		Log::warn("The queuing engine API reply does not contain the exit code");
		ok = false;
		return -1;
	}
	ok = true;
	return reply_obj.value("cmd_exit_code").toInt();
}

int QueuingEngineControllerGeneric::getEngineExitCode(QJsonDocument &reply_doc, bool &ok) const
{
	QJsonObject reply_obj = reply_doc.object();
	if (!reply_obj.contains("qe_exit_code"))
	{
		Log::warn("The queuing engine API reply does not contain the queuing engine exit code");
		ok = false;
		return -1;
	}
	ok = true;
	return reply_obj.value("qe_exit_code").toInt();
}

QString QueuingEngineControllerGeneric::getQueue(QJsonDocument &reply_doc, bool &ok) const
{
	QJsonObject reply_obj = reply_doc.object();
	if (!reply_obj.contains("queue"))
	{
		Log::warn("The queuing engine API reply does not contain the queue");
		ok = false;
		return "";
	}
	ok = true;
	return reply_obj.value("queue").toString();
}
