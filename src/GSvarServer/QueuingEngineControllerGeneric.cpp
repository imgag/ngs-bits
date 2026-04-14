#include "QueuingEngineControllerGeneric.h"
#include "HttpRequestHandler.h"
#include "Log.h"
#include "Settings.h"
#include "QueuingEngineApiHelper.h"

QueuingEngineControllerGeneric::QueuingEngineControllerGeneric()
	: proxy_(QNetworkProxy::NoProxy)
{
	qe_api_base_url_ = Settings::string("qe_api_base_url", true);
	secure_token_ = Settings::string("qe_secure_token", true);

	if (qe_api_base_url_.isEmpty())
	{
		THROW(ArgumentException, "Base URL for the queuing engine API is not set");
	}
	if (qe_api_base_url_.isEmpty())
	{
		THROW(ArgumentException, "Secure token for the queuing engine API is not set");
	}
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
		//this should not happen - we expect 200 if the API answers
		if (reply.status_code!=200) THROW(ProgrammingException, "Invalid status code: " + QString::number(reply.status_code));

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		checkReplyIsValid(reply_doc, job_id, "submit");

		int exit_code = reply_doc.object().value("exit_code").toInt();
		if (exit_code!=0)
		{
			QByteArrayList results = getResults(reply_doc);
			db.addAnalysisHistoryEntry(job_id, "error", results);
		}
		else
		{
			QString qe_job_id = reply_doc.object().value("qe_job_id").toString().trimmed();
			db.getQuery().exec("UPDATE analysis_job SET sge_id='" + qe_job_id + "' WHERE id="+QString::number(job_id));
			db.addAnalysisHistoryEntry(job_id, "started", QByteArrayList());
		}
	}
	catch(Exception e)
	{
		Log::error("There has been an error while submitting job '"+QString::number(job_id)+"': " + e.message());
	}
}

bool QueuingEngineControllerGeneric::updateRunningJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).updateRunningJob(job.sge_id, job.sge_queue);
		//this should not happen - we expect 200 if the API answers
		if (reply.status_code!=200) THROW(ProgrammingException, "Invalid status code: " + QString::number(reply.status_code));

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		checkReplyIsValid(reply_doc, job_id, "update");

		QString status = reply_doc.object().value("status").toString();
		if (status=="finished")
		{
			QByteArrayList results = getResults(reply_doc);
			db.addAnalysisHistoryEntry(job_id, "finished", results);
		}
		else if (status=="error")
		{
			 QByteArrayList results = getResults(reply_doc);
			 db.addAnalysisHistoryEntry(job_id, "error", results);
		}
		else
		{
			//nothing to do for 'started'
		}

		//update queue if necessary
		QString queue = reply_doc.object().value("queue").toString().trimmed();
		if (!queue.isEmpty())
		{
			QString queue_old = db.getValue("SELECT sge_queue FROM analysis_job WHERE id='" + QString::number(job_id)+"'").toString().trimmed();
			if (queue!=queue_old)
			{
				SqlQuery query = db.getQuery();
				query.prepare("UPDATE analysis_job SET sge_queue=:0 WHERE id=:1");
				query.bindValue(0, queue);
				query.bindValue(1, job_id);
				query.exec();
			}
		}
	}
	catch(Exception e)
	{
		Log::error("There has been an error while updating job '"+QString::number(job_id)+"': " + e.message());
	}

	return false; //always return false otherwise 'checkCompletedJob' is called
}

void QueuingEngineControllerGeneric::checkCompletedJob(NGSD&, QString, QByteArrayList, int) const
{
	//not needed for generic queuing engine
}

void QueuingEngineControllerGeneric::deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	try
	{
		ServerReply reply = QueuingEngineApiHelper(qe_api_base_url_, proxy_, secure_token_).deleteJob(job.sge_id, job.type);
		//this should not happen - we expect 200 if the API answers
		if (reply.status_code!=200) THROW(ProgrammingException, "Invalid status code: " + QString::number(reply.status_code));

		QJsonDocument reply_doc = QJsonDocument::fromJson(reply.body);
		checkReplyIsValid(reply_doc, job_id, "delete");

		int exit_code = reply_doc.object().value("exit_code").toInt();
		if (exit_code!=0) THROW(ArgumentException, "Exit code '" + QString::number(exit_code) + "' returned by API!");

		QByteArrayList results = getResults(reply_doc);
		db.addAnalysisHistoryEntry(job_id, "canceled", results);
	}
	catch(Exception e)
	{
		Log::error("There has been an error while deleting job '"+QString::number(job_id)+"': " + e.message());
	}
}

void QueuingEngineControllerGeneric::checkReplyIsValid(QJsonDocument &reply_doc, int job_id, QByteArray action) const
{
	if (!reply_doc.isObject())
	{
		THROW(ArgumentException, "API reply for action '"+action+"' of job ID '" + QString::number(job_id)+ "' not a valid JSON!");
	}

	QByteArrayList fields;
	fields << "result";
	if (action=="submit")
	{
		fields << "qe_job_id" << "exit_code";
	}
	else if (action=="update")
	{
		fields << "status" << "queue";
	}
	else if (action=="delete")
	{
		fields << "exit_code";
	}
	else THROW(ProgrammingException, "Unknown action '" + action +"'!");

	QJsonObject reply_obj = reply_doc.object();
	foreach(QByteArray field, fields)
	{
		//check fields are there
		if (!reply_obj.contains(field))
		{
			THROW(ArgumentException, "API reply for action '"+action+"' does not contain field '"+field+"'!");
		}

		//check field content
		if (action=="update" && field=="status")
		{
			QString status = reply_obj.value("status").toString();
			static QStringList valid = {"started", "finished", "error"};
			if (!valid.contains(status)) THROW(ArgumentException, "Invalid status '" + status +"'!");
		}
		if (action=="submit" && field=="qe_job_id")
		{
			QString qe_job_id = reply_obj.value("qe_job_id").toString().trimmed();
			if (qe_job_id.isEmpty() || qe_job_id.size()>10) THROW(ArgumentException, "Invalid queing engine job ID '"+qe_job_id+"'");
		}
	}
}

QByteArrayList QueuingEngineControllerGeneric::getResults(QJsonDocument &reply_doc) const
{
	QString result = reply_doc.object().value("result").toString();
	QByteArrayList results_as_list;
	for (const QString& line : result.split('\n'))
	{
		results_as_list << line.toUtf8();
	}
	return results_as_list;
}

