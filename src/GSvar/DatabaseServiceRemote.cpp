#include <QApplication>
#include <QMessageBox>
#include "DatabaseServiceRemote.h"
#include "Settings.h"
#include "ApiCaller.h"
#include "GUIHelper.h"

DatabaseServiceRemote::DatabaseServiceRemote()	
{
}

QString DatabaseServiceRemote::checkPassword(const QString user_name, const QString password) const
{
	return makePostApiCall("validate_credentials", RequestUrlParams(), QString("name="+user_name+"&password="+password).toUtf8(), false);
}

BedFile DatabaseServiceRemote::processingSystemRegions(int sys_id, bool ignore_if_missing) const
{
	BedFile output;
	RequestUrlParams params;
	params.insert("sys_id", QString::number(sys_id).toUtf8());
	QByteArray reply = makeGetApiCall("ps_regions", params, ignore_if_missing);

	if ((reply.length() == 0) && (!ignore_if_missing))
	{
		THROW(Exception, "Could not get the processing system regions for " + QString::number(sys_id));
	}

	output = output.fromText(reply);

	return output;
}

GeneSet DatabaseServiceRemote::processingSystemGenes(int sys_id, bool ignore_if_missing) const
{
	GeneSet output;
	RequestUrlParams params;
	params.insert("sys_id", QString::number(sys_id).toUtf8());
	QByteArray reply = makeGetApiCall("ps_genes", params, ignore_if_missing);

	if ((reply.length() == 0) && (!ignore_if_missing))
	{
		THROW(Exception, "Could not get the processing system genes for " + QString::number(sys_id));
	}

	output = GeneSet::createFromText(reply);

	return output;
}

QStringList DatabaseServiceRemote::secondaryAnalyses(QString processed_sample_name, QString analysis_type) const
{
	QStringList list;
	RequestUrlParams params;
	params.insert("ps_name", processed_sample_name.toUtf8());
	params.insert("type", analysis_type.toUtf8());
	QByteArray reply = makeGetApiCall("secondary_analyses", params, true);
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
	FileLocation output;
	RequestUrlParams params;
	params.insert("ps_id", processed_sample_id.toUtf8());
	params.insert("type", FileLocation::typeToString(type).toUtf8());
	QByteArray reply = makeGetApiCall("processed_sample_path", params, true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get a " + FileLocation::typeToString(type) + " file for the processed sample " + NGSD().processedSampleName(processed_sample_id));
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonArray json_array = json_doc.array();
	for (int i = 0; i < json_array.count(); i++)
	{
		if (!json_array.at(i).isObject()) break;

		if (json_array.at(i).toObject().contains("id") && json_array.at(i).toObject().contains("type") && json_array.at(i).toObject().contains("filename") && json_array.at(i).toObject().contains("exists"))
		{
			return FileLocation(json_array.at(i).toObject().value("id").toString(), FileLocation::stringToType(json_array.at(i).toObject().value("type").toString()), json_array.at(i).toObject().value("filename").toString(), json_array.at(i).toObject().value("exists").toBool());
		}
	}

	return output;
}

FileInfo DatabaseServiceRemote::analysisJobLatestLogInfo(const int& job_id) const
{
	RequestUrlParams params;
	params.insert("job_id", QString::number(job_id).toUtf8());
	QByteArray reply = makeGetApiCall("analysis_job_last_update", params, true);
	if (reply.length()>0)
	{
		QJsonDocument json_doc = QJsonDocument::fromJson(reply);
		QJsonObject json_object = json_doc.object();

		if (json_object.contains("latest_file") && json_object.contains("latest_file_with_path") && json_object.contains("latest_created") && json_object.contains("latest_mod"))
		{
			return FileInfo(json_object.value("latest_file").toString(),
							json_object.value("latest_file_with_path").toString(),
							QDateTime().fromSecsSinceEpoch(json_object.value("latest_created").toString().toLongLong()),
							QDateTime().fromSecsSinceEpoch(json_object.value("latest_mod").toString().toLongLong()));
		}
	}

	return FileInfo();
}

FileLocation DatabaseServiceRemote::analysisJobGSvarFile(const int& job_id) const
{
	RequestUrlParams params;
	params.insert("job_id", QString::number(job_id).toUtf8());
	QByteArray reply = makeGetApiCall("analysis_job_gsvar_file", params, true);
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
	RequestUrlParams params;
	params.insert("job_id", QString::number(job_id).toUtf8());
	QByteArray reply = makeGetApiCall("analysis_job_log", params, true);
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

QList<MultiSampleAnalysisInfo> DatabaseServiceRemote::getMultiSampleAnalysisInfo(QStringList& analyses) const
{
	QList<MultiSampleAnalysisInfo> result;
	QJsonArray json_in_array = QJsonArray::fromStringList(analyses);
	QJsonDocument json_in_doc(json_in_array);
    QByteArray reply = ApiCaller().post("multi_sample_analysis_info", RequestUrlParams(), HttpHeaders(), "analyses=" + json_in_doc.toJson().toPercentEncoding(), true, true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the list of analysis names");
	}

	QJsonDocument json_out_doc = QJsonDocument::fromJson(QUrl::fromPercentEncoding(reply).toUtf8());
	if (json_out_doc.isArray())
	{
		QJsonArray multi_sample_analysis_info_array = json_out_doc.array();
		for (int i = 0; i < multi_sample_analysis_info_array.count(); i++)
		{
			MultiSampleAnalysisInfo analysis_info;
			if (multi_sample_analysis_info_array[i].isObject())
			{
				QJsonObject info_object = multi_sample_analysis_info_array[i].toObject();
				analysis_info.analysis_file = info_object.value("analysis_file").toString();
				analysis_info.analysis_name = info_object.value("analysis_name").toString();

				if (info_object.value("ps_sample_name_list").isArray())
				{
					QJsonArray ps_sample_name_array = info_object.value("ps_sample_name_list").toArray();
					for (int ps_n = 0; ps_n < ps_sample_name_array.count(); ps_n++)
					{
						if (ps_sample_name_array[ps_n].isString())
						{
							analysis_info.ps_sample_name_list.append(ps_sample_name_array[ps_n].toString());
						}
					}
				}
				if (info_object.value("ps_sample_id_list").isArray())
				{
					QJsonArray ps_sample_id_array = info_object.value("ps_sample_id_list").toArray();
					for (int ps_i = 0; ps_i < ps_sample_id_array.count(); ps_i++)
					{
						if (ps_sample_id_array[ps_i].isString())
						{
							analysis_info.ps_sample_id_list.append(ps_sample_id_array[ps_i].toString());
						}
					}
				}
				result.append(analysis_info);
			}
		}
	}

	return result;
}

QStringList DatabaseServiceRemote::getRnaFusionPics(const QString& rna_id) const
{
	RequestUrlParams params;
	params.insert("rna_id", rna_id.toUtf8());
	QByteArray reply = makeGetApiCall("rna_fusion_pics", params, true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the list of RNA fusion plots for " + rna_id);
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonArray json_array = json_doc.array();

	QStringList output;
	for (int i = 0; i < json_array.count(); i++)
	{
		 output << json_array[i].toString();
	}

	return output;
}

QStringList DatabaseServiceRemote::getRnaExpressionPlots(const QString& rna_id) const
{
	RequestUrlParams params;
	params.insert("rna_id", rna_id.toUtf8());
	QByteArray reply = makeGetApiCall("rna_expression_plots", params, true);
	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the list of RNA expression plots for " + rna_id);
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(reply);
	QJsonArray json_array = json_doc.array();

	QStringList output;
	for (int i = 0; i < json_array.count(); i++)
	{
		 output << json_array[i].toString();
	}

	return output;
}

QByteArray DatabaseServiceRemote::makeGetApiCall(QString api_path, RequestUrlParams params, bool ignore_if_missing) const
{		
	try
	{
		HttpHeaders headers;
		headers.insert("Content-Type", "text/plain");
		return ApiCaller().get(api_path, params, headers, true, false, true);
	}
	catch (Exception& e)
	{
		if (!ignore_if_missing) QMessageBox::warning(GUIHelper::mainWindow(), "Database API GET call error", e.message());
	}

	return QByteArray{};
}

QByteArray DatabaseServiceRemote::makePostApiCall(QString api_path, RequestUrlParams params, QByteArray content, bool ignore_if_missing) const
{
	try
	{
		return ApiCaller().post(api_path, params, HttpHeaders(), content, false, false, true);
	}
	catch (Exception& e)
	{
		if (!ignore_if_missing) QMessageBox::warning(GUIHelper::mainWindow(), "Database API POST call error", e.message());
	}

	return QByteArray{};
}
