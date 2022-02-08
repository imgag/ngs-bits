#include "DatabaseServiceRemote.h"
#include "Settings.h"
#include "HttpRequestHandler.h"
#include "Helper.h"
#include <QApplication>
#include <QMessageBox>

DatabaseServiceRemote::DatabaseServiceRemote()
	: enabled_(Settings::boolean("NGSD_enabled"))
{
}

bool DatabaseServiceRemote::enabled() const
{
	return enabled_;
}

BedFile DatabaseServiceRemote::processingSystemRegions(int sys_id, bool ignore_if_missing) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	BedFile output;
	QByteArray reply = makeApiCall("ps_regions?sys_id="+QString::number(sys_id), ignore_if_missing);

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
	QByteArray reply = makeApiCall("ps_amplicons?sys_id="+QString::number(sys_id), ignore_if_missing);

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
	QByteArray reply = makeApiCall("ps_genes?sys_id="+QString::number(sys_id), ignore_if_missing);

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
	QByteArray reply = makeApiCall("secondary_analyses?ps_name="+processed_sample_name+"&type="+analysis_type, true);
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
	QByteArray reply = makeApiCall("processed_sample_path?ps_id=" + processed_sample_id + "&type=" + FileLocation::typeToString(type), true);

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

FileLocation DatabaseServiceRemote::analysisJobGSvarFile(const int& job_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	FileLocation output;
	QByteArray reply = makeApiCall("analysis_job_gsvar_file?job_id=" + QString::number(job_id), true);

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get a GSvar file for the job id " + QString::number(job_id));
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

QByteArray DatabaseServiceRemote::makeApiCall(QString url_param, bool ignore_if_missing) const
{
	HttpHeaders add_headers;
	add_headers.insert("Accept", "text/plain");
	add_headers.insert("User-Agent", "GSvar");
	QByteArray result;

	try
	{
		result = HttpRequestHandler(HttpRequestHandler::NONE).get(Helper::serverApiUrl() +url_param, add_headers);
	}
	catch (Exception& e)
	{
		if (!ignore_if_missing)
		{
			QMessageBox::warning(QApplication::activeWindow(), "Database API call error", e.message());
		}
	}

	return result;
}
