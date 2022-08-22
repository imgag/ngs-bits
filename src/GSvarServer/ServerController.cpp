#include "ServerController.h"
#include "FileLocationProviderLocal.h"
#include "ToolBase.h"
#include "Statistics.h"

ServerController::ServerController()
{
}

HttpResponse ServerController::serveResourceAsset(const HttpRequest& request)
{
	QString path_lower = request.getPath().toLower().trimmed();
	if (path_lower == "favicon.ico")
	{
		return EndpointController::serveStaticFile(":/assets/client/favicon.ico", request.getMethod(), request.getContentType(), request.getHeaders());
	}
	else if (path_lower.isEmpty() || path_lower=="index" || path_lower.startsWith("index."))
	{
		return EndpointController::serveStaticFile(":/assets/client/info.html", request.getMethod(), request.getContentType(), request.getHeaders());
	}
	else if (path_lower=="info")
	{
		QString text = "{"
					   "\"name\": \"" + ToolBase::applicationName() + "\","
					   "\"description\": \"GSvar server\","
					   "\"version\": \"" + ToolBase::version() + "\","
					   "\"api_version\": \"" + NGSHelper::serverApiVersion() + "\""
					   "}";
		BasicResponseData response_data;
		response_data.length = text.length();
		response_data.content_type = request.getContentType();
		response_data.is_downloadable = false;

		return HttpResponse(response_data, text.toLocal8Bit());
	}
	else if (path_lower=="bam")
	{
		QString filename;
		if (request.getPathItems().count() > 0) filename = request.getPathItems()[0];
		if (!filename.isEmpty())
		{
			return EndpointController::serveStaticFile(":/assets/client/" + filename, request.getMethod(), request.getContentType(), request.getHeaders());
		}
	}

	return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Requested asset was not found");
}

HttpResponse ServerController::locateFileByType(const HttpRequest& request)
{
	qDebug() << "File location service";
	if (!request.getUrlParams().contains("ps_url_id"))
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Sample id has not been provided");
	}
	QString ps_url_id = request.getUrlParams()["ps_url_id"];

	UrlEntity url_entity = UrlManager::getURLById(ps_url_id.trimmed());	
	QString found_file = url_entity.filename_with_path;
	if (!QFile::exists(found_file))
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processed sample file does not exist");
	}

	bool return_if_missing = true;
	if (request.getUrlParams().contains("return_if_missing"))
	{
		if (request.getUrlParams()["return_if_missing"] == "0")
		{
			return_if_missing = false;
		}
	}

	bool multiple_files = true;
	if (request.getUrlParams().contains("multiple_files"))
	{
		if (request.getUrlParams()["multiple_files"].trimmed() == "0")
		{
			multiple_files = false;
		}
	}

	FileLocationList file_list {};
	QJsonDocument json_doc_output {};
	QJsonArray json_list_output {};
	PathType requested_type = FileLocation::stringToType(request.getUrlParams()["type"].toUpper().trimmed());

	if (found_file.isEmpty())
	{
		file_list << FileLocation("undefined", requested_type, "noname", false);
	}
	else
	{
		VariantList variants;
		variants.loadHeaderOnly(found_file);
		FileLocationProviderLocal* file_locator = new FileLocationProviderLocal(found_file, variants.getSampleHeader(), variants.type());

		switch(requested_type)
		{
			case PathType::VCF:
				if (multiple_files)
				{
					file_list = file_locator->getVcfFiles(return_if_missing);
					break;
				}
				file_list << file_locator->getAnalysisVcf();
				break;
			case PathType::STRUCTURAL_VARIANTS:
				file_list << file_locator->getAnalysisSvFile();
				break;
			case PathType::MOSAIC_VARIANTS:
				file_list << file_locator->getAnalysisMosaicFile();
				break;
			case PathType::COPY_NUMBER_CALLS:
				if (multiple_files)
				{
					file_list = file_locator->getCopyNumberCallFiles(return_if_missing);
					break;
				}				
				file_list << file_locator->getAnalysisCnvFile();
				break;
			case PathType::COPY_NUMBER_CALLS_MOSAIC:
				file_list << file_locator->getAnalysisMosaicCnvFile();
				break;
			case PathType::UPD:
				file_list << file_locator->getAnalysisUpdFile();
				break;
			case PathType::REPEAT_EXPANSION_IMAGE:
				if (!request.getUrlParams().contains("locus"))
				{
					return HttpResponse(ResponseStatus::BAD_REQUEST, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Locus value has not been provided");
				}
				file_list << file_locator->getRepeatExpansionImage(request.getUrlParams()["locus"]);
				break;
			case PathType::BAM:
				file_list = file_locator->getBamFiles(return_if_missing);
				break;
			case PathType::COPY_NUMBER_RAW_DATA:
				if (multiple_files)
				{
					file_list = file_locator->getCnvCoverageFiles(return_if_missing);
					break;
				}				
				file_list << file_locator->getSomaticCnvCoverageFile();
				break;
			case PathType::BAF:
				file_list = file_locator->getBafFiles(return_if_missing);
				break;
			case PathType::MANTA_EVIDENCE:
				file_list = file_locator->getMantaEvidenceFiles(return_if_missing);
				break;
			case PathType::CIRCOS_PLOT:
				file_list = file_locator->getCircosPlotFiles(return_if_missing);
				break;
			case PathType::REPEAT_EXPANSIONS:
				file_list = file_locator->getRepeatExpansionFiles(return_if_missing);
				break;
			case PathType::PRS:
				file_list = file_locator->getPrsFiles(return_if_missing);
				break;
			case PathType::LOWCOV_BED:
				if (multiple_files)
				{
					file_list = file_locator->getLowCoverageFiles(return_if_missing);
					break;
				}				
				file_list << file_locator->getSomaticLowCoverageFile();
				break;
			case PathType::ROH:
				file_list = file_locator->getRohFiles(return_if_missing);
				break;
			case PathType::CNV_RAW_DATA_CALL_REGIONS:
				file_list << file_locator->getSomaticCnvCallFile();
				break;
			case PathType::MSI:
				file_list << file_locator->getSomaticMsiFile();
				break;
			case PathType::QC:
				file_list = file_locator->getQcFiles();
				break;
			case PathType::EXPRESSION:
				file_list = file_locator->getExpressionFiles(return_if_missing);
				break;
			case PathType::IGV_SCREENSHOT:
				file_list << file_locator->getSomaticIgvScreenshotFile();
				break;
			default:
				FileLocation gsvar_file(
					url_entity.file_id,
					PathType::GSVAR,
					found_file,
					true
				);
				file_list.append(gsvar_file);
		}
	}

	for (int i = 0; i < file_list.count(); ++i)
	{
		QJsonObject cur_json_item {};
		cur_json_item.insert("id", file_list[i].id);
		cur_json_item.insert("type", FileLocation::typeToString(file_list[i].type));
		bool needs_url = true;
		if (request.getUrlParams().contains("path"))
		{
			if (request.getUrlParams()["path"].toLower() == "absolute") needs_url = false;
		}
		if ((needs_url) && (file_list[i].exists))
		{
			try
			{
				bool return_http = false;
				if (requested_type == PathType::BAM)
				{
					return_http = true;
				}
				cur_json_item.insert("filename", createFileTempUrl(file_list[i].filename, request.getUrlParams()["token"], return_http));
			}
			catch (Exception& e)
			{
				return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), e.message());
			}
		}
		else
		{
			cur_json_item.insert("filename", file_list[i].filename);
		}
		cur_json_item.insert("exists", QFile::exists(file_list[i].filename));

		json_list_output.append(cur_json_item);
	}

	json_doc_output.setArray(json_list_output);

	BasicResponseData response_data;
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getProcessedSamplePath(const HttpRequest& request)
{
	qDebug() << "Processed sample path";

	PathType type = PathType::GSVAR;
	if (request.getUrlParams().contains("type"))
	{
		type = FileLocation::stringToType(request.getUrlParams()["type"].toUpper().trimmed());
	}

	QString ps_name;
	QString found_file_path;
	try
	{
		NGSD db;
		int id = request.getUrlParams()["ps_id"].toInt();
		ps_name = db.processedSampleName(request.getUrlParams()["ps_id"]);
		Session current_session = SessionManager::getSessionBySecureToken(request.getUrlParams()["token"]);

		//access restricted only for user role 'user_restricted'
		QString role = db.getValue("SELECT user_role FROM user WHERE id='" + QString::number(current_session.user_id) + "'").toString().toLower();
		if (role=="user_restricted")
		{
			if (!db.userCanAccess(current_session.user_id, id))
			{
				return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "You do not have permissions to open this sample");
			}
		}

		found_file_path =  db.processedSamplePath(QString::number(id), type);
	}
	catch (Exception& e)
	{
		Log::error("Error opening processed sample from NGSD:" + e.message());
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), e.message());
	}

	bool return_http = false;
	if (type == PathType::BAM) return_http = true;

	FileLocation project_file = FileLocation(ps_name, type, createFileTempUrl(found_file_path, request.getUrlParams()["token"], return_http), QFile::exists(found_file_path));

	QJsonDocument json_doc_output;
	QJsonArray file_location_as_json_list;
	QJsonObject file_location_as_json_object;
	file_location_as_json_object.insert("id", ps_name);
	file_location_as_json_object.insert("type", project_file.typeAsString());
	file_location_as_json_object.insert("filename", project_file.filename);
	file_location_as_json_object.insert("exists", project_file.exists);
	file_location_as_json_list.append(file_location_as_json_object);
	json_doc_output.setArray(file_location_as_json_list);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getAnalysisJobGSvarFile(const HttpRequest& request)
{	
	qDebug() << "Analysis job GSvar file";

	QString ps_name;
	QString found_file_path;

	try
	{
		NGSD db;
		int job_id = request.getUrlParams()["job_id"].toInt();
		AnalysisJob job = db.analysisInfo(job_id, true);
		ps_name = db.processedSampleName(db.processedSampleId(job.samples[0].name));
		found_file_path = db.analysisJobGSvarFile(job_id);
	}
	catch (Exception& e)
	{
		Log::error("Error while looking for the analysis job GSvar file in NGSD:" + e.message());
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), e.message());
	}

	FileLocation analysis_job_gsvar_file = FileLocation(ps_name, PathType::GSVAR, createFileTempUrl(found_file_path, request.getUrlParams()["token"], false), QFile::exists(found_file_path));

	QJsonDocument json_doc_output;
	QJsonObject file_location_as_json_object;

	file_location_as_json_object.insert("id", ps_name);
	file_location_as_json_object.insert("type", analysis_job_gsvar_file.typeAsString());
	file_location_as_json_object.insert("filename", analysis_job_gsvar_file.filename);
	file_location_as_json_object.insert("exists", analysis_job_gsvar_file.exists);
	json_doc_output.setObject(file_location_as_json_object);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getAnalysisJobLastUpdate(const HttpRequest& request)
{
	int job_id = request.getUrlParams()["job_id"].toInt();
	QString last_update;
	QJsonDocument json_doc_output;
	QJsonObject last_update_as_json_object;
	FileInfo log_info = NGSD().analysisJobLatestLogInfo(job_id);

	if (!log_info.isEmpty())
	{
		last_update_as_json_object.insert("latest_file", log_info.file_name);
		last_update_as_json_object.insert("latest_file_with_path", log_info.file_name_with_path);
		last_update_as_json_object.insert("latest_mod", QString::number(log_info.last_modiefied.toSecsSinceEpoch()));
		last_update_as_json_object.insert("latest_created", QString::number(log_info.created.toSecsSinceEpoch()));
	}
	json_doc_output.setObject(last_update_as_json_object);

	BasicResponseData response_data;
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getAnalysisJobLog(const HttpRequest& request)
{
	qDebug() << "Analysis job log file";
	FileLocation analysis_job_log_file;

	try
	{
		NGSD db;
		int job_id = request.getUrlParams()["job_id"].toInt();
		AnalysisJob job = db.analysisInfo(job_id, true);
		QString id = db.processedSampleName(db.processedSampleId(job.samples[0].name));
		QString log = NGSD().analysisJobLatestLogInfo(job_id).file_name_with_path;

		analysis_job_log_file = FileLocation(id, PathType::OTHER, createFileTempUrl(log, request.getUrlParams()["token"], false), QFile::exists(log));
	}
	catch (Exception& e)
	{
		Log::error("Error while looking for the analysis job log file:" + e.message());
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), e.message());
	}

	QJsonDocument json_doc_output;
	QJsonObject file_location_as_json_object;

	file_location_as_json_object.insert("id", analysis_job_log_file.id);
	file_location_as_json_object.insert("type", analysis_job_log_file.typeAsString());
	file_location_as_json_object.insert("filename", analysis_job_log_file.filename);
	file_location_as_json_object.insert("exists", analysis_job_log_file.exists);
	json_doc_output.setObject(file_location_as_json_object);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::saveProjectFile(const HttpRequest& request)
{
	QString ps_url_id = request.getUrlParams()["ps_url_id"];
	UrlEntity url = UrlManager::getURLById(ps_url_id);

	if (url.filename_with_path.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "The GSvar file in " + ps_url_id + "could not be located");
	}

	QJsonDocument json_doc;
	try
	{
		json_doc = QJsonDocument::fromJson(request.getBody());
	}
	catch (Exception& e)
	{
		Log::error("Error while parsing changes for the GSvar file" + url.filename_with_path + ":" + e.message());
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Changes for the GSvar file in " + ps_url_id + "could not be parsed: " + e.message());
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
				return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not identify key columns in GSvar file: " + ps_url_id);
			}
			continue;
		}

		QList<QString> line_columns = line.split("\t");
		QString variant_in = line_columns[chr_pos] + ":" + line_columns[start_pos] + "-" + line_columns[end_pos] + " " + line_columns[ref_pos] + ">" + line_columns[obs_pos];
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
						return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not identify changed column " + column + " in GSvar file: " + ps_url_id);
					}
					is_current_variant_changed = true;
					is_file_changed = true;

					line_columns[column_names.indexOf(column)] = QUrl::toPercentEncoding(text); // text.replace("\n", " ").replace("\t", " ");
				}
			}
			catch (Exception& e)
			{
				qDebug() << "Error while processing changes for the GSvar file" + url.filename_with_path + ":" << e.message();
				return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Changes for the GSvar file in " + ps_url_id + "could not be parsed: " + e.message());
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

	in_file.data()->close();
	out_file.data()->close();

	if (is_file_changed)
	{
		//remove original file
		if (!in_file.data()->remove())
		{
			Log::warn("Could not remove: " + in_file.data()->fileName());
		}
		//put the changed copy instead of the original
		if (!out_file.data()->rename(url.filename_with_path))
		{
			Log::warn("Could not rename: " + out_file.data()->fileName());
		}
	}

	if (is_file_changed)
	{
		return HttpResponse(ResponseStatus::OK, request.getContentType(), "Project file has been changed");
	}
	return HttpResponse(ResponseStatus::OK, request.getContentType(), "No changes to the file detected");
}

HttpResponse ServerController::saveQbicFiles(const HttpRequest& request)
{
	QString qbic_data_path = Settings::string("qbic_data_path");
	if (!QDir(qbic_data_path).exists())
	{
		QDir(qbic_data_path).mkpath(".");
	}
	if (!qbic_data_path.endsWith(QDir::separator())) qbic_data_path = qbic_data_path + QDir::separator();

	QString filename = request.getUrlParams()["filename"];
	QString folder_name = request.getUrlParams()["id"];
	QString content = request.getBody();

	if ((filename.isEmpty()) || (folder_name.isEmpty()))
	{
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Path or filename has not been provided");
	}

	// It should not be possible to move up to the parent directory or to access system directories
	folder_name = folder_name.replace(".", "");
	folder_name = folder_name.replace(QDir::separator(), "");
	folder_name = qbic_data_path + folder_name;

	if (!QDir(folder_name).exists())
	{		
		QDir(folder_name).mkpath(".");
	}

	if (!folder_name.endsWith(QDir::separator())) folder_name = folder_name + QDir::separator();

	try
	{
		QSharedPointer<QFile> qBicFile = Helper::openFileForWriting(folder_name+filename);
		QTextStream stream(qBicFile.data());
		stream << content;
		qBicFile->close();
	}
	catch (Exception& e)
	{
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not save the data: " + e.message());
	}

	return HttpResponse(ResponseStatus::OK, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), filename + " has been saved");
}


HttpResponse ServerController::uploadFile(const HttpRequest& request)
{
	qDebug() << "File upload request";	

	if ((request.getFormDataParams().contains("ps_url_id")) && (!request.getMultipartFileName().isEmpty()))
	{		
		QString ps_url_id = request.getFormDataParams()["ps_url_id"];
		UrlEntity url_entity = UrlManager::getURLById(ps_url_id.trimmed());
		if (!url_entity.path.isEmpty())
		{
			if (!url_entity.path.endsWith("/")) url_entity.path = url_entity.path + "/";
			qDebug() << "url_entity.path + request.getMultipartFileName()" << url_entity.path + request.getMultipartFileName();
			qDebug() << "request.getMultipartFileName()" << request.getMultipartFileName();
			QSharedPointer<QFile> outfile = Helper::openFileForWriting(url_entity.path + request.getMultipartFileName());
			outfile->write(request.getMultipartFileContent());
			return HttpResponse(ResponseStatus::OK, request.getContentType(), "File has been uploaded");
		}
		else
		{
			return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processed sample path has not been found");
		}
	}

	return HttpResponse(ResponseStatus::BAD_REQUEST, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Parameters have not been provided");
}

HttpResponse ServerController::calculateLowCoverage(const HttpRequest& request)
{
	qDebug() << "Low coverage calcualtion";
	BedFile roi;
	QString bam_file_name;
	int cutoff = 0;

	if (request.getFormUrlEncoded().contains("roi"))
	{
		roi = roi.fromText(request.getFormUrlEncoded()["roi"].toLocal8Bit());
	}
	if (request.getFormUrlEncoded().contains("bam_url_id"))
	{
		bam_file_name = UrlManager::getURLById(request.getFormUrlEncoded()["bam_url_id"]).filename_with_path;
	}
	if (request.getFormUrlEncoded().contains("cutoff"))
	{
		cutoff = request.getFormUrlEncoded()["cutoff"].toInt();
	}

	BedFile low_cov = Statistics::lowCoverage(roi, bam_file_name, cutoff);

	if(!low_cov.toText().isEmpty())
	{
		QByteArray body = low_cov.toText().toLocal8Bit();

		BasicResponseData response_data;
		response_data.length = body.length();
		response_data.content_type = request.getContentType();
		response_data.is_downloadable = false;
		return HttpResponse(response_data, body);
	}
	return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), "Low coverage regions are empty");
}

HttpResponse ServerController::calculateAvgCoverage(const HttpRequest& request)
{
	qDebug() << "Average coverage calcualtion";
	BedFile low_cov;
	QString bam_file_name;

	if (request.getFormUrlEncoded().contains("low_cov"))
	{
		low_cov = low_cov.fromText(request.getFormUrlEncoded()["low_cov"].toLocal8Bit());
	}
	if (request.getFormUrlEncoded().contains("bam_url_id"))
	{
		bam_file_name = UrlManager::getURLById(request.getFormUrlEncoded()["bam_url_id"]).filename_with_path;
	}

	Statistics::avgCoverage(low_cov, bam_file_name, 1, false);
	if(!low_cov.toText().isEmpty())
	{
		QByteArray body = low_cov.toText().toLocal8Bit();

		BasicResponseData response_data;
		response_data.length = body.length();
		response_data.content_type = request.getContentType();
		response_data.is_downloadable = false;
		return HttpResponse(response_data, body);
	}
	return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), "Average coverage for gaps is empty");
}

HttpResponse ServerController::calculateTargetRegionReadDepth(const HttpRequest& request)
{
	qDebug() << "Target region read depth calcualtion";
	BedFile regions;
	QString bam_file_name;

	if (request.getFormUrlEncoded().contains("regions"))
	{
		regions = regions.fromText(request.getFormUrlEncoded()["regions"].toLocal8Bit());
	}
	if (request.getFormUrlEncoded().contains("bam_url_id"))
	{
		bam_file_name = UrlManager::getURLById(request.getFormUrlEncoded()["bam_url_id"]).filename_with_path;
	}

	QString ref_file = Settings::string("reference_genome");
	QCCollection stats = Statistics::mapping(regions, bam_file_name, ref_file);

	for (int i=0; i<stats.count(); ++i)
	{
		if (stats[i].accession()=="QC:2000025")
		{
			QByteArray body = stats[i].toString().toLocal8Bit();
			BasicResponseData response_data;
			response_data.length = body.length();
			response_data.content_type = request.getContentType();
			response_data.is_downloadable = false;
			return HttpResponse(response_data, body);

		}
	}

	return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), "Could not calculate target region read depth");
}

HttpResponse ServerController::getMultiSampleAnalysisInfo(const HttpRequest& request)
{
	qDebug() << "List analysis names";
	if (request.getFormUrlEncoded().contains("analyses"))
	{
		QJsonDocument json_in_doc = QJsonDocument::fromJson(QUrl::fromPercentEncoding(request.getFormUrlEncoded()["analyses"].toLocal8Bit()).toLocal8Bit());
		if (json_in_doc.isArray())
		{
			QJsonArray json_in_array = json_in_doc.array();
			QJsonArray multi_sample_analysis_info_array;
			for (int i = 0; i < json_in_array.count(); i++)
			{
				VariantList vl;
				QString file_url = json_in_array[i].toString();
				QStringList file_url_parts = file_url.split("/");
				if (file_url_parts.count() < 2) continue;
				QString url_id = file_url_parts[file_url_parts.count()-2];
				UrlEntity url = UrlManager::getURLById(url_id);
				vl.loadHeaderOnly(url.filename_with_path);

				QJsonObject multi_sample_analysis_info_object;
				multi_sample_analysis_info_object.insert("analysis_file", file_url);
				multi_sample_analysis_info_object.insert("analysis_name", vl.analysisName());

				QJsonArray ps_sample_name_array;
				QJsonArray ps_sample_id_array;
				foreach(const SampleInfo& info, vl.getSampleHeader())
				{					
					ps_sample_name_array.append(info.id);
					ps_sample_id_array.append(NGSD().processedSampleId(info.id));
				}
				multi_sample_analysis_info_object.insert("ps_sample_name_list", ps_sample_name_array);
				multi_sample_analysis_info_object.insert("ps_sample_id_list", ps_sample_id_array);
				multi_sample_analysis_info_array.append(multi_sample_analysis_info_object);
			}

			QByteArray body = QJsonDocument(multi_sample_analysis_info_array).toJson();
			BasicResponseData response_data;
			response_data.length = body.length();
			response_data.content_type = request.getContentType();
			response_data.is_downloadable = false;
			return HttpResponse(response_data, body);
		}
	}
	return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), "Could not get any information about the multi-sample analysis");
}

HttpResponse ServerController::performLogin(const HttpRequest& request)
{
	qDebug() << "Login request";
	if (!request.getFormUrlEncoded().contains("name") || !request.getFormUrlEncoded().contains("password"))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "No username or/and password were found");
	}

	NGSD db;
	QString message = db.checkPassword(request.getFormUrlEncoded()["name"], request.getFormUrlEncoded()["password"]);
	if (message.isEmpty())
	{
		QString secure_token = ServerHelper::generateUniqueStr();
		Session cur_session = Session(db.userId(request.getFormUrlEncoded()["name"]), QDateTime::currentDateTime(), false);

		SessionManager::addNewSession(secure_token, cur_session);
		QByteArray body = secure_token.toLocal8Bit();

		BasicResponseData response_data;
		response_data.length = body.length();
		response_data.content_type = request.getContentType();
		response_data.is_downloadable = false;
		qDebug() << "User creadentials are valid";
		return HttpResponse(response_data, body);
	}

	return HttpResponse(ResponseStatus::UNAUTHORIZED, request.getContentType(), "Invalid username or password");
}

HttpResponse ServerController::getSessionInfo(const HttpRequest& request)
{
	if (request.getUrlParams().contains("token"))
	{
		QString token = request.getUrlParams()["token"];
		Session current_session = SessionManager::getSessionBySecureToken(token);

		QJsonDocument json_doc;
		QJsonObject json_object;

		json_object.insert("user_id", current_session.user_id);
		json_object.insert("login_time", current_session.login_time.toSecsSinceEpoch());
		json_object.insert("is_db_token", current_session.is_for_db_only);
		json_object.insert("valid_period", ServerHelper::getNumSettingsValue("session_duration"));
		json_doc.setObject(json_object);

		BasicResponseData response_data;
		response_data.length = json_doc.toJson().length();
		response_data.content_type = request.getContentType();
		response_data.is_downloadable = false;		
		return HttpResponse(response_data, json_doc.toJson());
	}
	return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "You are not allowed to access this information");
}

HttpResponse ServerController::validateCredentials(const HttpRequest& request)
{
	qDebug() << "Validation of user credentials";	
	QString message = NGSD().checkPassword(request.getFormUrlEncoded()["name"], request.getFormUrlEncoded()["password"]);

	QByteArray body = message.toLocal8Bit();
	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;

	return HttpResponse(response_data, body);
}

HttpResponse ServerController::getDbToken(const HttpRequest& request)
{
	Session user_session = SessionManager::getSessionBySecureToken(request.getFormUrlEncoded()["token"]);

	if (user_session.isEmpty())
	{
		return HttpResponse(ResponseStatus::UNAUTHORIZED, request.getContentType(), "You need to log in first");
	}

	Session cur_session = Session(user_session.user_id, QDateTime::currentDateTime(), true);
	QString db_token = ServerHelper::generateUniqueStr();
	SessionManager::addNewSession(db_token, cur_session);
	QByteArray body = db_token.toLocal8Bit();

	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, body);
}

HttpResponse ServerController::getNgsdCredentials(const HttpRequest& request)
{
	qDebug() << "NGSD credentials request";
	QJsonDocument json_doc;
	QJsonObject json_object;

	QString prefix = "ngsd";
	json_object.insert(prefix + "_host", Settings::string(prefix + "_host", true));
	json_object.insert(prefix + "_port", Settings::string(prefix + "_port", true));
	json_object.insert(prefix + "_name", Settings::string(prefix + "_name", true));
	json_object.insert(prefix + "_user", Settings::string(prefix + "_user", true));
	json_object.insert(prefix + "_pass", Settings::string(prefix + "_pass", true));
	json_doc.setObject(json_object);

	BasicResponseData response_data;
	response_data.length = json_doc.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, json_doc.toJson());
}

HttpResponse ServerController::getGenlabCredentials(const HttpRequest& request)
{
	qDebug() << "Genlab credentials request";
	QJsonDocument json_doc;
	QJsonObject json_object;

	json_object.insert("genlab_mssql", Settings::boolean("genlab_mssql", true));	
	json_object.insert("genlab_host", Settings::string("genlab_host", true));
	json_object.insert("genlab_port", Settings::string("genlab_port", true));
	json_object.insert("genlab_name", Settings::string("genlab_name", true));
	json_object.insert("genlab_user", Settings::string("genlab_user", true));
	json_object.insert("genlab_pass", Settings::string("genlab_pass", true));
	json_doc.setObject(json_object);

	BasicResponseData response_data;
	response_data.length = json_doc.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, json_doc.toJson());
}

HttpResponse ServerController::performLogout(const HttpRequest& request)
{
	QByteArray body {};
	if (!request.getFormUrlEncoded().contains("token"))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "Secure token has not been provided");
	}
	if (SessionManager::isTokenReal(request.getFormUrlEncoded()["token"]))
	{
		try
		{
			SessionManager::removeSession(request.getFormUrlEncoded()["token"]);
		} catch (Exception& e)
		{
			return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), e.message());
		}
		body = request.getFormUrlEncoded()["token"].toLocal8Bit();

		BasicResponseData response_data;
		response_data.length = body.length();
		response_data.content_type = request.getContentType();
		response_data.is_downloadable = false;

		return HttpResponse(response_data, body);
	}
	return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "You have provided an invalid token");
}

HttpResponse ServerController::getProcessingSystemRegions(const HttpRequest& request)
{
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = NGSD().processingSystemRegionsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processing system regions file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getProcessingSystemAmplicons(const HttpRequest& request)
{
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = NGSD().processingSystemAmpliconsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processing system amplicons file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getProcessingSystemGenes(const HttpRequest& request)
{
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = NGSD().processingSystemGenesFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processing system genes file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getSecondaryAnalyses(const HttpRequest& request)
{
	QString processed_sample_name = request.getUrlParams()["ps_name"];
	QString type  = QUrl::fromEncoded(request.getUrlParams()["type"].toLatin1()).toString();
	QStringList secondary_analyses;
	try
	{
		secondary_analyses = NGSD().secondaryAnalyses(processed_sample_name, type);
	}
	catch (DatabaseException& e)
	{
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not get secondary analyses from the database");
	}

	QJsonDocument json_doc_output;
	QJsonArray json_array;
	for (int i = 0; i < secondary_analyses.count(); i++)
	{
		json_array.append(createFileTempUrl(secondary_analyses[i], request.getUrlParams()["token"], false));
	}
	json_doc_output.setArray(json_array);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

QString ServerController::createFileTempUrl(const QString& file, const QString& token, const bool& return_http)
{
	QString id = ServerHelper::generateUniqueStr();
	UrlManager::addNewUrl(id, UrlEntity(QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file, id, QDateTime::currentDateTime()));

	return NGSHelper::serverApiUrl(return_http) + "temp/" + id + "/" + QFileInfo(file).fileName() + "?token=" + token;
}
