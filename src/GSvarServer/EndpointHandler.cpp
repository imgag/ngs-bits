#include "EndpointHandler.h"

EndpointHandler::EndpointHandler()
{
}

HttpResponse EndpointHandler::serveIndexPage(const HttpRequest& request)
{
	if (request.getPrefix().toLower() == "favicon.ico")
	{
		return serveFavicon(request);
	}
	else if ((request.getPrefix().toLower().contains("index") || (request.getPrefix().toLower().trimmed() == "v1")) && (request.getPathParams().count() == 0))
	{
		return EndpointController::serveStaticFile(":/assets/client/info.html", request.getMethod(), request.getHeaders());
	}

	return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), "Requested page was not found");
}

HttpResponse EndpointHandler::serveFavicon(const HttpRequest& request)
{
	if (request.getPathParams().count() == 0)
	{
		return EndpointController::serveStaticFile(":/assets/client/favicon.ico", request.getMethod(), request.getHeaders());
	}
	return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), "Favicon was not found");
}

HttpResponse EndpointHandler::serveApiInfo(const HttpRequest& request)
{
	if (request.getPathParams().count() == 0)
	{
		return EndpointController::serveStaticFile(":/assets/client/api.json", request.getMethod(), request.getHeaders());
	}
	return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), "API info was not found");
}

HttpResponse EndpointHandler::locateFileByType(const HttpRequest& request)
{
	qDebug() << "File location service";
	if (!request.getUrlParams().contains("ps_url_id"))
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, request.getContentType(), "Sample id has not been provided");
	}
	QString ps_url_id = request.getUrlParams().value("ps_url_id");

	UrlEntity url_entity = UrlManager::getURLById(ps_url_id.trimmed());	
	QString found_file = url_entity.filename_with_path;

	bool return_if_missing = true;
	if (!request.getUrlParams().contains("return_if_missing"))
	{
		if (request.getUrlParams().value("return_if_missing") == "0")
		{
			return_if_missing = false;
		}
	}

	if (found_file.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), "Could not find the sample: " + request.getUrlParams().value("ps_url_id"));
	}

	VariantList variants;
	variants.load(found_file);

	FileLocationProviderLocal* file_locator = new FileLocationProviderLocal(found_file, variants.getSampleHeader(), variants.type());

	QList<FileLocation> file_list {};
	QJsonDocument json_doc_output {};
	QJsonArray json_list_output {};

	if(request.getUrlParams()["type"].toLower() == "analysisvcf")
	{
		file_list << file_locator->getAnalysisVcf();
	}
	else if(request.getUrlParams()["type"].toLower() == "analysissv")
	{
		file_list << file_locator->getAnalysisSvFile();
	}
	else if(request.getUrlParams()["type"].toLower() == "analysiscnv")
	{
		file_list << file_locator->getAnalysisCnvFile();
	}
	else if(request.getUrlParams()["type"].toLower() == "analysismosaiccnv")
	{
		file_list << file_locator->getAnalysisMosaicCnvFile();
	}
	else if(request.getUrlParams()["type"].toLower() == "analysisupd")
	{
		file_list << file_locator->getAnalysisUpdFile();
	}
	else if(request.getUrlParams()["type"].toLower() == "repeatexpansionimage")
	{
		if (!request.getUrlParams().contains("locus"))
		{
			return HttpResponse(ResponseStatus::BAD_REQUEST, request.getContentType(), "Locus value has not been provided");
		}
		file_list << file_locator->getRepeatExpansionImage(request.getUrlParams().value("locus"));
	}
	else if(request.getUrlParams()["type"].toLower() == "bam")
	{
		file_list = file_locator->getBamFiles(return_if_missing);
	}
	else if(request.getUrlParams()["type"].toLower() == "cnvcoverage")
	{
		file_list = file_locator->getCnvCoverageFiles(return_if_missing);
	}
	else if(request.getUrlParams()["type"].toLower() == "baf")
	{
		file_list = file_locator->getBafFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "mantaevidence")
	{
		file_list = file_locator->getMantaEvidenceFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "circosplot")
	{
		file_list = file_locator->getCircosPlotFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "vcf")
	{
		file_list = file_locator->getVcfFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "repeatexpansion")
	{
		file_list = file_locator->getRepeatExpansionFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "prs")
	{
		file_list = file_locator->getPrsFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "lowcoverage")
	{
		file_list = file_locator->getLowCoverageFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "copynumbercall")
	{
		file_list = file_locator->getCopyNumberCallFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "roh")
	{
		file_list = file_locator->getRohFiles(return_if_missing);
	}
	else if (request.getUrlParams()["type"].toLower() == "somaticcnvcoverage")
	{
		file_list << file_locator->getSomaticCnvCoverageFile();
	}
	else if (request.getUrlParams()["type"].toLower() == "somaticcnvcall")
	{
		file_list << file_locator->getSomaticCnvCallFile();
	}
	else if (request.getUrlParams()["type"].toLower() == "somaticlowcoverage")
	{
		file_list << file_locator->getSomaticLowCoverageFile();
	}
	else if (request.getUrlParams()["type"].toLower() == "somaticmsi")
	{
		file_list << file_locator->getSomaticMsiFile();
	}
	else
	{
		FileLocation gsvar_file(
			url_entity.file_id,
			PathType::GSVAR,
			found_file,
			true
		);
		file_list.append(gsvar_file);
	}

	for (int i = 0; i < file_list.count(); ++i)
	{
		qDebug() << file_list[i].filename;
		QJsonObject cur_json_item {};
		cur_json_item.insert("id", file_list[i].id);
		cur_json_item.insert("type", FileLocation::typeToString(file_list[i].type));
		bool needs_url = true;
		if (request.getUrlParams().contains("path"))
		{
			if (request.getUrlParams()["path"].toLower() == "absolute") needs_url = false;

		}
		if (needs_url)
		{
			try
			{
				cur_json_item.insert("filename", createFileTempUrl(file_list[i].filename));
			}
			catch (Exception& e)
			{
				return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), e.message());
			}
		}
		else
		{
			cur_json_item.insert("filename", file_list[i].filename);
		}

		cur_json_item.insert("exists", file_list[i].exists);
		json_list_output.append(cur_json_item);
	}

	json_doc_output.setArray(json_list_output);

	BasicResponseData response_data;
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = ContentType::APPLICATION_JSON;
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse EndpointHandler::locateProjectFile(const HttpRequest& request)
{
	qDebug() << "Project file location";
	QList<QString> project_files;
	QJsonDocument json_doc_output;
	QJsonArray json_list_output;
	QJsonObject json_object_output;

	QString id;
	QString found_file_path;
	try
	{
		id = NGSD().processedSampleName(request.getUrlParams().value("ps_id"));
		found_file_path =  NGSD().processedSamplePath(request.getUrlParams().value("ps_id"), PathType::GSVAR);
	}
	catch (Exception& e)
	{
		qWarning() << "Error opening processed sample from NGSD:" + e.message();
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), e.message());
	}

	FileLocation project_file = FileLocation(id, PathType::GSVAR, createFileTempUrl(found_file_path), true);
	json_object_output.insert("id", id);
	json_object_output.insert("type", project_file.typeAsString());
	json_object_output.insert("filename", project_file.filename);
	json_object_output.insert("exists", project_file.exists);
	json_list_output.append(json_object_output);
	json_doc_output.setArray(json_list_output);

	BasicResponseData response_data;
	response_data.byte_range = ByteRange{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = ContentType::APPLICATION_JSON;
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse EndpointHandler::saveProjectFile(const HttpRequest& request)
{
	QString ps_url_id = request.getUrlParams()["ps_url_id"];
	UrlEntity url = UrlManager::getURLById(ps_url_id);

	if (url.filename_with_path.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, "The GSvar file in " + ps_url_id + "could not be located");
	}

	QJsonDocument json_doc;
	try
	{
		json_doc = QJsonDocument::fromJson(request.getBody());
	}
	catch (Exception& e)
	{
		qWarning() << "Error while parsing changes for the GSvar file" + url.filename_with_path + ":" << e.message();
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Changes for the GSvar file in " + ps_url_id + "could not be parsed: " + e.message());
	}

	QString tmp = url.filename_with_path + "_" + ps_url_id + ".tmp";
	QSharedPointer<QFile> in_file = Helper::openFileForReading(url.filename_with_path);
	QTextStream in_stream(in_file.data());
	QSharedPointer<QFile> out_file = Helper::openFileForWriting(tmp);
	QTextStream out_stream(out_file.data());

	QList<QString> column_names;
	int chr_pos = -1;
	int start_pos = -1;
	int end_pos = -1;
	int ref_pos = -1;
	int obs_pos = -1;
	bool is_file_changed = false;
	while(!in_stream.atEnd())
	{
		QString line = in_stream.readLine();
		if (line.startsWith("##"))
		{
			out_stream << line << "\n";
			continue;
		}

		// Headers
		if ((line.startsWith("#")) && (line.count("#") == 1))
		{
			out_stream << line << "\n";
			column_names = line.split("\t");
			chr_pos = column_names.indexOf("#chr");
			start_pos = column_names.indexOf("start");
			end_pos = column_names.indexOf("end");
			ref_pos = column_names.indexOf("ref");
			obs_pos = column_names.indexOf("obs");

			if ((chr_pos == -1) || (start_pos == -1) || (end_pos == -1) || (ref_pos == -1) || (obs_pos == -1))
			{
				return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Could not identify key columns in GSvar file: " + ps_url_id);
			}
			continue;
		}

		QList<QString> line_columns = line.split("\t");
		QString variant_in = line_columns.value(chr_pos) + ":" + line_columns.value(start_pos) + "-" + line_columns.value(end_pos) + " " + line_columns.value(ref_pos) + ">" + line_columns.value(obs_pos);
		bool is_current_variant_changed = false;

		for (int i = 0; i <  json_doc.array().size(); i++)
		{
			try
			{
				QString variant_changed = json_doc.array().takeAt(i).toObject().value("variant").toString().trimmed();
				QString column = json_doc.array().takeAt(i).toObject().value("column").toString().trimmed();
				QString text = json_doc.array().takeAt(i).toObject().value("text").toString();

				// Locating changed variant
				if (variant_in.toLower().trimmed() == variant_changed.toLower())
				{
					// Locating changed column
					if (column_names.indexOf(column) == -1)
					{
						return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Could not identify changed column " + column + " in GSvar file: " + ps_url_id);
					}
					is_current_variant_changed = true;
					is_file_changed = true;
					line_columns[column_names.indexOf(column)] = text;
				}
			}
			catch (Exception& e)
			{
				qDebug() << "Error while processing changes for the GSvar file" + url.filename_with_path + ":" << e.message();
				return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Changes for the GSvar file in " + ps_url_id + "could not be parsed: " + e.message());
			}
		}

		if (!is_current_variant_changed)
		{
			out_stream << line << "\n";
		}
		else
		{
			out_stream << line_columns.join("\t") << "\n";
		}
	}

	if (is_file_changed)
	{
		qDebug() << "Temporary GSvar file: " << url.filename_with_path;
		qDebug() << tmp;
		//copy temp
		QFile::remove(url.filename_with_path);
		QFile::rename(tmp, url.filename_with_path);
	}

	return HttpResponse(ResponseStatus::OK, ContentType::APPLICATION_JSON, "");
}

HttpResponse EndpointHandler::getProcessingSystemRegions(const HttpRequest& request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemRegionsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, "Processing system regions file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse EndpointHandler::getProcessingSystemAmplicons(const HttpRequest& request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemAmpliconsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, "Processing system amplicons file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse EndpointHandler::getProcessingSystemGenes(const HttpRequest& request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemGenesFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, "Processing system genes file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

QString EndpointHandler::createFileTempUrl(const QString& file)
{
	QString id = ServerHelper::generateUniqueStr();
	UrlManager::addUrlToStorage(id, QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file);
	return ServerHelper::getStringSettingsValue("server_host") +
			+ ":" + ServerHelper::getStringSettingsValue("server_port") +
			+ "/v1/temp/" + id + "/" + QFileInfo(file).fileName();
}
