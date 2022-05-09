#include "ServerController.h"
#include "FileLocationProviderLocal.h"
#include "ToolBase.h"

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
		response_data.content_type = ContentType::APPLICATION_JSON;
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
	if (SessionManager::isUserSessionExpired(request.getUrlParams()["token"]))
	{
		return HttpResponse(ResponseStatus::REQUEST_TIMEOUT, request.getContentType(), "Secure token has expired");
	}

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
	response_data.content_type = ContentType::APPLICATION_JSON;
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

	QJsonDocument json_doc_output;
	QJsonArray json_list_output;
	QJsonObject json_object_output;

	QString id;
	QString found_file_path;
	try
	{
		NGSD db;
		id = db.processedSampleName(request.getUrlParams()["ps_id"]);
		Session current_session = SessionManager::getSessionBySecureToken(request.getUrlParams()["token"]);

		//access restricted only for user role 'user_restricted'
		QString role = db.getValue("SELECT user_role FROM user WHERE id='" + QString::number(current_session.user_id) + "'").toString().toLower();
		if (role=="user_restricted")
		{
			if (!db.userCanAccess(current_session.user_id, id.toInt()))
			{
				return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "You do not have permissions to open this sample");
			}
		}

		found_file_path =  db.processedSamplePath(id, type);
	}
	catch (Exception& e)
	{
		Log::error("Error opening processed sample from NGSD:" + e.message());
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), e.message());
	}

	bool return_http = false;
	if (type == PathType::BAM)
	{
		return_http = true;
	}

	FileLocation project_file = FileLocation(id, type, createFileTempUrl(found_file_path, request.getUrlParams()["token"], return_http), QFile::exists(found_file_path));

	json_object_output.insert("id", id);
	json_object_output.insert("type", project_file.typeAsString());
	json_object_output.insert("filename", project_file.filename);
	json_object_output.insert("exists", project_file.exists);
	json_list_output.append(json_object_output);
	json_doc_output.setArray(json_list_output);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = ContentType::APPLICATION_JSON;
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getAnalysisJobGSvarFile(const HttpRequest& request)
{	
	qDebug() << "Analysis job GSvar file";
	QJsonDocument json_doc_output;
	QJsonObject json_object_output;

	QString id;
	QString found_file_path;

	try
	{
		NGSD db;
		int job_id = request.getUrlParams()["job_id"].toInt();
		AnalysisJob job = db.analysisInfo(job_id, true);
		id = db.processedSampleName(db.processedSampleId(job.samples[0].name));
		found_file_path = db.analysisJobGSvarFile(job_id);
	}
	catch (Exception& e)
	{
		Log::error("Error while looking for the analysis job GSvar file in NGSD:" + e.message());
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), e.message());
	}

	FileLocation analysis_job_gsvar_file = FileLocation(id, PathType::GSVAR, createFileTempUrl(found_file_path, request.getUrlParams()["token"], false), QFile::exists(found_file_path));

	json_object_output.insert("id", id);
	json_object_output.insert("type", analysis_job_gsvar_file.typeAsString());
	json_object_output.insert("filename", analysis_job_gsvar_file.filename);
	json_object_output.insert("exists", analysis_job_gsvar_file.exists);
	json_doc_output.setObject(json_object_output);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = ContentType::APPLICATION_JSON;
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

	QString msg = "Size = " + QString::number(json_doc.array().size()) + request.getBody();

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
		return HttpResponse(ResponseStatus::OK, ContentType::APPLICATION_JSON, "changed" + msg);
	}
	return HttpResponse(ResponseStatus::OK, ContentType::APPLICATION_JSON, msg);
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
			return HttpResponse(ResponseStatus::OK, request.getContentType(), "OK");
		}
		else
		{
			return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processed sample path has not been found");
		}
	}

	return HttpResponse(ResponseStatus::BAD_REQUEST, request.getContentType(), "Parameters have not been provided");
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
		Session cur_session = Session(db.userId(request.getFormUrlEncoded()["name"]), QDateTime::currentDateTime());

		SessionManager::addNewSession(secure_token, cur_session);
		QByteArray body = secure_token.toLocal8Bit();

		BasicResponseData response_data;
		response_data.length = body.length();
		response_data.content_type = ContentType::TEXT_PLAIN;
		response_data.is_downloadable = false;
		qDebug() << "User creadentials are valid";
		return HttpResponse(response_data, body);
	}

	return HttpResponse(ResponseStatus::UNAUTHORIZED, request.getContentType(), "Invalid username or password");
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
		response_data.content_type = ContentType::TEXT_PLAIN;
		response_data.is_downloadable = false;

		return HttpResponse(response_data, body);
	}
	return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "You have provided an invalid token");
}

HttpResponse ServerController::getProcessingSystemRegions(const HttpRequest& request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemRegionsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processing system regions file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getProcessingSystemAmplicons(const HttpRequest& request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemAmpliconsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processing system amplicons file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getProcessingSystemGenes(const HttpRequest& request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemGenesFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Processing system genes file has not been found");
	}
	return EndpointController::createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getSecondaryAnalyses(const HttpRequest& request)
{
	NGSD db;
	QString processed_sample_name = request.getUrlParams()["ps_name"];
	QString type  = QUrl::fromEncoded(request.getUrlParams()["type"].toLatin1()).toString();
	QStringList secondary_analyses;
	try
	{
		secondary_analyses = db.secondaryAnalyses(processed_sample_name, type);
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
	response_data.content_type = ContentType::APPLICATION_JSON;
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

QString ServerController::createFileTempUrl(const QString& file, const QString& token, const bool& return_http)
{
	QString id = ServerHelper::generateUniqueStr();
	UrlManager::addUrlToStorage(id, QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file);

	return NGSHelper::serverApiUrl(return_http) + "temp/" + id + "/" + QFileInfo(file).fileName() + "?token=" + token;
}
