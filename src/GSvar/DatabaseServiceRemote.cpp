#include "DatabaseServiceRemote.h"
#include "Settings.h"
#include "HttpRequestHandler.h"
#include "NGSHelper.h"
#include <QApplication>
#include <QMessageBox>

DatabaseServiceRemote::DatabaseServiceRemote()
	: enabled_(true)
{
}

bool DatabaseServiceRemote::enabled() const
{
	return enabled_;
}

QString DatabaseServiceRemote::checkPassword(const QString user_name, const QString password) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	return makePostApiCall("validate_credentials", "name="+user_name+"&password="+password, false);
}

BedFile DatabaseServiceRemote::processingSystemRegions(int sys_id, bool ignore_if_missing) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	BedFile output;
	QByteArray reply = makeGetApiCall("ps_regions?sys_id="+QString::number(sys_id), ignore_if_missing);

	if ((reply.length() == 0) && (!ignore_if_missing))
	{
		THROW(Exception, "Could not get the processing system regions for " + QString::number(sys_id));
	}

	output = output.fromText(reply);

	return output;
}

BedFile DatabaseServiceRemote::processingSystemAmplicons(int sys_id, bool ignore_if_missing) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	BedFile output;
	QByteArray reply = makeGetApiCall("ps_amplicons?sys_id="+QString::number(sys_id), ignore_if_missing);

	if ((reply.length() == 0) && (!ignore_if_missing))
	{
		THROW(Exception, "Could not get the processing system amplicons for " + QString::number(sys_id));
	}

	output = output.fromText(reply);

	return output;
}

GeneSet DatabaseServiceRemote::processingSystemGenes(int sys_id, bool ignore_if_missing) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	GeneSet output;
	QByteArray reply = makeGetApiCall("ps_genes?sys_id="+QString::number(sys_id), ignore_if_missing);

	if ((reply.length() == 0) && (!ignore_if_missing))
	{
		THROW(Exception, "Could not get the processing system genes for " + QString::number(sys_id));
	}

	output = GeneSet::createFromText(reply);

	return output;
}

QStringList DatabaseServiceRemote::secondaryAnalyses(QString processed_sample_name, QString analysis_type) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	QStringList list;
	QByteArray reply = makeGetApiCall("secondary_analyses?ps_name="+processed_sample_name+"&type="+analysis_type, true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the processing system genes for " + processed_sample_name);
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonArray json_array = json_doc.array();
	QStringList analyses;
	for (int i = 0; i < json_array.count(); i++)
	{
		if (!json_array.at(i).isString()) break;
		list.append(json_array.at(i).toString());
	}

	return list;
}

FileLocation DatabaseServiceRemote::processedSamplePath(const QString& processed_sample_id, PathType type) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	FileLocation output;
	QByteArray reply = makeGetApiCall("processed_sample_path?ps_id=" + processed_sample_id + "&type=" + FileLocation::typeToString(type), true);

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get a GSvar file for the processed sample " + processed_sample_id);
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonArray json_array = json_doc.array();
	QStringList analyses;
	for (int i = 0; i < json_array.count(); i++)
	{
		if (!json_array.at(i).isObject()) break;

		if (json_array.at(i).toObject().contains("id") && json_array.at(i).toObject().contains("type")
			&& json_array.at(i).toObject().contains("filename") && json_array.at(i).toObject().contains("exists"))
		{
			return FileLocation(json_array.at(i).toObject().value("id").toString(), FileLocation::stringToType(json_array.at(i).toObject().value("type").toString()),
								json_array.at(i).toObject().value("filename").toString(), json_array.at(i).toObject().value("exists").toBool());
		}
	}

	return output;
}

FileInfo DatabaseServiceRemote::analysisJobLatestLogInfo(const int& job_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	FileInfo output;
	QByteArray reply = makeGetApiCall("analysis_job_last_update?job_id=" + QString::number(job_id), true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the latest update info for the job id " + QString::number(job_id));
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonObject json_object = json_doc.object();

	if (json_object.contains("latest_file") && json_object.contains("latest_file_with_path") && json_object.contains("latest_created") && json_object.contains("latest_mod"))
	{
		return FileInfo(json_object.value("latest_file").toString(),
						json_object.value("latest_file_with_path").toString(),
						QDateTime().fromSecsSinceEpoch(json_object.value("latest_created").toString().toLongLong()),
						QDateTime().fromSecsSinceEpoch(json_object.value("latest_mod").toString().toLongLong()));
	}

	return output;
}

FileLocation DatabaseServiceRemote::analysisJobGSvarFile(const int& job_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	QByteArray reply = makeGetApiCall("analysis_job_gsvar_file?job_id=" + QString::number(job_id), true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get a GSvar file for the job id " + QString::number(job_id));
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonObject json_object = json_doc.object();

	if (json_object.contains("id") && json_object.contains("type") && json_object.contains("filename") && json_object.contains("exists"))
	{
		return FileLocation(json_object.value("id").toString(), FileLocation::stringToType(json_object.value("type").toString()), json_object.value("filename").toString(), json_object.value("exists").toBool());
	}

	return FileLocation{};
}

FileLocation DatabaseServiceRemote::analysisJobLogFile(const int& job_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	QByteArray reply = makeGetApiCall("analysis_job_log?job_id=" + QString::number(job_id), true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get a log file for the job id " + QString::number(job_id));
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonObject json_object = json_doc.object();

	if (json_object.contains("id") && json_object.contains("type") && json_object.contains("filename") && json_object.contains("exists"))
	{
		return FileLocation(json_object.value("id").toString(), FileLocation::stringToType(json_object.value("type").toString()), json_object.value("filename").toString(), json_object.value("exists").toBool());
	}

	return FileLocation{};
}

HttpHeaders DatabaseServiceRemote::defaultHeaders() const
{
	HttpHeaders add_headers;
	add_headers.insert("Accept", "text/plain");
	add_headers.insert("User-Agent", "GSvar");
	return add_headers;
}

QString DatabaseServiceRemote::getTokenIfExists() const
{
	try
	{
		return "&token=" + LoginManager::userToken();
	}
	catch (ProgrammingException& e)
	{
		qDebug() << e.message();
	}
	return "";
}

QByteArray DatabaseServiceRemote::makeGetApiCall(QString url_param, bool ignore_if_missing) const
{	
	try
	{		
		HttpHeaders get_headers = defaultHeaders();
		get_headers.insert("Content-Type", "text/plain");
		return HttpRequestHandler(HttpRequestHandler::NONE).get(NGSHelper::serverApiUrl() + url_param + getTokenIfExists(), get_headers);
	}
	catch (Exception& e)
	{
		if (!ignore_if_missing)
		{
			QMessageBox::warning(QApplication::activeWindow(), "Database API GET call error", e.message());
		}
	}

	return QByteArray{};
}

QByteArray DatabaseServiceRemote::makePostApiCall(QString url_param, QString content, bool ignore_if_missing) const
{
	try
	{
		HttpHeaders post_headers = defaultHeaders();
		post_headers.insert("Content-Type", "application/x-www-form-urlencoded");
		return HttpRequestHandler(HttpRequestHandler::NONE).post(NGSHelper::serverApiUrl() + url_param + getTokenIfExists(), content.toLocal8Bit(), post_headers);
	}
	catch (Exception& e)
	{
		if (!ignore_if_missing)
		{
			QMessageBox::warning(QApplication::activeWindow(), "Database API POST call error", e.message());
		}
	}

	return QByteArray{};
}
