#include "DatabaseServiceRemote.h"
#include "Settings.h"
#include "HttpRequestHandler.h"

DatabaseServiceRemote::DatabaseServiceRemote()
	: enabled_(Settings::boolean("NGSD_enabled"))
{
}

bool DatabaseServiceRemote::enabled() const
{
	return enabled_;
}

BedFile DatabaseServiceRemote::processingSystemRegions(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	BedFile output;
	QByteArray reply = makeApiCall("ps_regions?sys_id="+QString::number(sys_id));

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the processing system regions for " + QString::number(sys_id));
	}
	else
	{
		output.fromText(reply);
	}

	return output;
}

BedFile DatabaseServiceRemote::processingSystemAmplicons(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	BedFile output;
	QByteArray reply = makeApiCall("ps_amplicons?sys_id="+QString::number(sys_id));

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the processing system amplicons for " + QString::number(sys_id));
	}
	else
	{
		output.fromText(reply);
	}

	return output;
}

GeneSet DatabaseServiceRemote::processingSystemGenes(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	GeneSet output;
	QByteArray reply = makeApiCall("ps_genes?sys_id="+QString::number(sys_id));

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the processing system genes for " + QString::number(sys_id));
	}
	else
	{
		output = GeneSet::createFromText(reply);
	}

	return output;
}

QStringList DatabaseServiceRemote::secondaryAnalyses(QString processed_sample_name, QString analysis_type) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	QStringList list;
	QByteArray reply = makeApiCall("secondary_analyses?ps_name="+processed_sample_name+"&type="+analysis_type);
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
	qDebug() << "processed_sample_id " << processed_sample_id;
	QByteArray reply = makeApiCall("processed_sample_path?ps_id=" + processed_sample_id + "&type=" + FileLocation::typeToString(type));

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

QByteArray DatabaseServiceRemote::makeApiCall(QString url_param) const
{
	HttpHeaders add_headers;
	add_headers.insert("Accept", "text/plain");
	return HttpRequestHandler(HttpRequestHandler::NONE).get(
			"https://" + Settings::string("server_host",true) + ":" + Settings::string("https_server_port")
			+ "/v1/"+url_param, add_headers);
}
