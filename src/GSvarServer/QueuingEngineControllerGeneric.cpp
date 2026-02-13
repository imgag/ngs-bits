#include "QueuingEngineControllerGeneric.h"
#include "HttpRequestHandler.h"
#include "Log.h"
#include "Settings.h"

QueuingEngineControllerGeneric::QueuingEngineControllerGeneric()
	: proxy_(QNetworkProxy::NoProxy)
{
	qe_api_base_url_ = Settings::string("qe_api_base_url", true);
	secure_token_ = Settings::string("qe_secure_token", true);

	//TODO check if URL and token are provided. Throw exception otherwise!
}

QString QueuingEngineControllerGeneric::getEngineName() const
{
	return "Generic";
}

void QueuingEngineControllerGeneric::submitJob(NGSD &db, int threads, QStringList queues, QStringList pipeline_args, QString working_directory, QString script, int job_id) const
{
	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).submitJob(threads, queues, pipeline_args, working_directory, script);
		if (reply.status_code!=200)
		{
			Log::error("There has been an error while submitting a job, response code: " + QString::number(reply.status_code)); //TODO throw
		}

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		checkReplyIsValid(reply_doc, job_id, "submit");

		int exit_code = getCommandExitCode(reply_doc);
		bool ok = false;
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
	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).updateRunningJob(job.sge_id, job.sge_queue);
		if (reply.status_code!=200)
		{
			Log::error("There has been an error while updating a job, response code: " + QString::number(reply.status_code));
		}

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		checkReplyIsValid(reply_doc, job_id, "update");

		int exit_code = getCommandExitCode(reply_doc);

		bool ok = false;
		QString status = getStatus(reply_doc, ok);
		if (!ok) return false; //TODO

		if (exit_code==0 && status=="queued/running")
		{
			job_finished = false;
			QString queue = getQueue(reply_doc, ok);
			if (!ok) return false; //TODO

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

	return false; //always return false otherwise 'checkCompletedJob' is called
}

void QueuingEngineControllerGeneric::checkCompletedJob(NGSD &db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{
}

void QueuingEngineControllerGeneric::deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).deleteJob(job.sge_id, job.type);
		if (reply.status_code!=200)
		{
			Log::error("There has been an error while deleting a job, response code: " + QString::number(reply.status_code));

		}

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		checkReplyIsValid(reply_doc, job_id, "delete");

		QByteArrayList results = getResults(reply_doc);
		db.addAnalysisHistoryEntry(job_id, "canceled", results);
	}
	catch(Exception e)
	{
		Log::error("There has been an error while deleting a job: " + e.message());
	}
}

void QueuingEngineControllerGeneric::checkReplyIsValid(QJsonDocument &reply_doc, int job_id, QByteArray action) const
{
	if (!reply_doc.isObject())
	{
		THROW(ArgumentException, "API reply for action '"+action+"' of job ID '" + QString::number(job_id)+ "' not a valid JSON!");
	}

	QByteArrayList fields;
	fields << "result" << "exit_code";
	if (action=="submit")
	{
		fields << "qe_job_id";
	}
	else if (action=="update")
	{
		fields << "status" << "queue";
	}
	else if (action=="delete")
	{
		//no extra fields
	}
	else THROW(ProgrammingException, "Unknown action '" + action +"'!");

	QJsonObject reply_obj = reply_doc.object();
	foreach(QByteArray field, fields)
	{
		if (!reply_obj.contains("result"))
		{
			THROW(ArgumentException, "API reply for action '"+action+"' of job ID '" + QString::number(job_id)+ "' does not contain field '"+field+"'!");
		}
	}
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
	return reply_obj.value("qe_job_id").toString().trimmed(); //TODO check that it is not empty, and not larger than 10 characters
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

int QueuingEngineControllerGeneric::getCommandExitCode(QJsonDocument &reply_doc) const
{
	QJsonObject reply_obj = reply_doc.object();

	bool ok = false;
	int exit_code = reply_obj.value("exit_code").toInt(&ok);
	if (!ok)

	return exit_code;
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
