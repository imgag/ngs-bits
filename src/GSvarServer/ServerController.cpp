#include "ServerController.h"
#include "PipelineSettings.h"
#include "FileMetaCache.h"
#include <QUrl>
#include <QProcess>

ServerController::ServerController()
{
}

HttpResponse ServerController::serveEndpointHelp(const HttpRequest& request)
{
	QByteArray body;
	if (request.getPathItems().count() == 2)
	{
		// Locate endpoint by URL and request method
		QList<Endpoint> selected_endpoints = {EndpointManager::getEndpointByUrlAndMethod(request.getPathItems()[0], HttpUtils::getMethodTypeFromString(request.getPathItems()[1]))};
		body = EndpointManager::getEndpointHelpTemplate(selected_endpoints).toUtf8();
	}
	else if (request.getPathItems().count() == 1)
	{
		// For the same URL several request methods may be used: e.g. GET and POST
		body = EndpointManager::getEndpointHelpTemplate(EndpointManager::getEndpointsByUrl(request.getPathItems()[0])).toUtf8();
	}
	else
	{
		// Help for all defined endpoints
		body = EndpointManager::getEndpointHelpTemplate(EndpointManager::getEndpointEntities()).toUtf8();
	}

	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, body);
}

HttpResponse ServerController::serveStaticFromServerRoot(const HttpRequest& request)
{
    return createStaticLocationResponse(findPathForServerFolder(request.getPathItems(), Settings::string("server_root", true)), request);
}

HttpResponse ServerController::serveStaticServerGenomes(const HttpRequest& request)
{
	return createStaticLocationResponse(findPathForServerFolder(request.getPathItems(), PipelineSettings::dataFolder() + "/genomes/"), request);
}

HttpResponse ServerController::serveStaticFromTempUrl(const HttpRequest& request)
{
	return createStaticLocationResponse(findPathForTempUrl(request.getPathItems()), request);
}

HttpResponse ServerController::createStaticFileRangeResponse(const QString& filename, const QList<ByteRange>& byte_ranges, const ContentType& type, bool is_downloadable)
{
	quint64 total_length = 0;
	for (int i = 0; i < byte_ranges.count(); ++i)
	{
		total_length = total_length + byte_ranges[i].length;
	}

    FastFileInfo *info = new FastFileInfo(filename);

	BasicResponseData response_data;
	response_data.filename = filename;
	response_data.length = total_length;
	response_data.byte_ranges = byte_ranges;
    response_data.file_size = info->size();
	response_data.is_stream = true;
	response_data.content_type = type;
	response_data.status = ResponseStatus::PARTIAL_CONTENT;
	response_data.is_downloadable = is_downloadable;

	return HttpResponse(response_data);
}

HttpResponse ServerController::createStaticStreamResponse(const QString& filename, bool is_downloadable)
{
    FastFileInfo *info = new FastFileInfo(filename);

    BasicResponseData response_data;
    response_data.length = info->size();
	response_data.filename = filename;
	response_data.file_size = response_data.length;
	response_data.is_stream = true;
	response_data.content_type = HttpUtils::getContentTypeByFilename(filename);
	response_data.is_downloadable = is_downloadable;

	return HttpResponse(response_data);
}

HttpResponse ServerController::createStaticFileResponse(const QString& filename, const HttpRequest& request)
{
    FastFileInfo *info = new FastFileInfo(filename);
    if ((filename.isEmpty()) || ((!filename.isEmpty()) && (!info->exists())))
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Requested file does not exist: " + filename));
		// Special case, when sending HEAD request for a file that does not exist
		if (request.getMethod() == RequestMethod::HEAD)
		{
			return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), 0.0);
		}

        return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), EndpointManager::formatResponseMessage(request, "Requested file could not be found"));
	}

    quint64 file_size = info->size();
	// Client wants to see only the size of the requested file (not its content)
	if (request.getMethod() == RequestMethod::HEAD)
	{
		return HttpResponse(ResponseStatus::OK, HttpUtils::getContentTypeByFilename(filename), file_size);
	}

	// Random read functionality based on byte-range headers
	if (request.getHeaders().contains("range"))
	{
		QList<ByteRange> byte_ranges;
		for (int i = 0; i < request.getHeaders()["range"].count(); ++i)
		{
			ByteRange current_range;
			current_range.start = 0;
			current_range.end = 0;

			QString range_value = request.getHeaders()["range"][i];
			// We support only bytes as units for range requests
			// Examples:
			// Range: bytes=200-1000, 2000-6576, 19000-
			// Range: bytes=200-1000
			// or (from position 19000 till the end of file)
			// Range: bytes=19000-
			// or (the last 500 bytes of the file)
			// // Range: bytes=-500
			range_value = range_value.replace("bytes", "");
			range_value = range_value.replace("=", "");
			range_value = range_value.trimmed();
			if (range_value.count("-") > 0)
			{
				bool is_start_set = true;
				bool is_end_set = true;
				if (range_value.mid(0, range_value.indexOf("-")).trimmed().length() == 0)
				{
					is_start_set = false;
				}
				if (range_value.mid(range_value.indexOf("-")+1, range_value.length()-range_value.indexOf("-")).trimmed().length() == 0)
				{
					is_end_set = false;
				}

				current_range.start = static_cast<quint64>(range_value.mid(0, range_value.indexOf("-")).trimmed().toULongLong());
				current_range.end = static_cast<quint64>(range_value.mid(range_value.indexOf("-")+1, range_value.length()-range_value.indexOf("-")).trimmed().toULongLong());

				if ((!is_start_set) && (is_end_set))
				{
					if (current_range.end<=(file_size-1))
					{

						current_range.start = file_size - 1 - current_range.end;
						current_range.end = file_size - 1;
					}
					else
                    {
                        return HttpResponse(ResponseStatus::RANGE_NOT_SATISFIABLE, request.getContentType(), EndpointManager::formatResponseMessage(request, "Range is outside the file boundary"));
					}
				}
				if ((!is_end_set) && (is_start_set))
				{
					current_range.end = file_size - 1;
				}

				if ((!is_start_set) && (!is_end_set))
                {
                    return HttpResponse(ResponseStatus::RANGE_NOT_SATISFIABLE, request.getContentType(), EndpointManager::formatResponseMessage(request, "Range limits have not been specified"));
				}

				if (current_range.start > current_range.end)
                {
                    return HttpResponse(ResponseStatus::RANGE_NOT_SATISFIABLE, request.getContentType(), EndpointManager::formatResponseMessage(request, "The requested range start position is greater than its end position"));
				}
			}

			current_range.length = ((current_range.end - current_range.start) > 0) ? (current_range.end - current_range.start) : 0;
			current_range.length = current_range.length + 1;
			byte_ranges.append(current_range);
		}
		if (hasOverlappingRanges(byte_ranges))
        {
            return HttpResponse(ResponseStatus::RANGE_NOT_SATISFIABLE, request.getContentType(), EndpointManager::formatResponseMessage(request, "Overlapping ranges have been detected"));
		}

		return createStaticFileRangeResponse(filename, byte_ranges, HttpUtils::getContentTypeByFilename(filename), false);
	}


	// Stream the content of the entire file
	return createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::serveResourceAsset(const HttpRequest& request)
{
	QString path_lower = request.getPath().toLower().trimmed();
	if (path_lower == "favicon.ico")
	{
		return createStaticFileResponse(":/assets/client/favicon.ico", request);
	}
	else if (path_lower.isEmpty() || path_lower=="index" || path_lower.startsWith("index."))
	{
		return createStaticFileResponse(":/assets/client/info.html", request);
	}
	else if (path_lower=="info")
	{
		QJsonDocument json_doc;
		QJsonObject json_object;

		json_object.insert("name", ToolBase::applicationName());
		json_object.insert("description", "GSvar server");
		json_object.insert("version", ToolBase::version());
		json_object.insert("api_version", ClientHelper::serverApiVersion());
		json_object.insert("start_time", ServerHelper::getServerStartDateTime().toSecsSinceEpoch());
        json_object.insert("server_url", Settings::string("server_host", true));
        json_doc.setObject(json_object);

		BasicResponseData response_data;
		response_data.length = json_doc.toJson().length();
		response_data.content_type = request.getContentType();
		response_data.is_downloadable = false;

		return HttpResponse(response_data, json_doc.toJson());
	}
	else if (path_lower=="bam")
	{
		QString filename;
		if (request.getPathItems().count() > 0) filename = request.getPathItems()[0];
		if (!filename.isEmpty())
		{
			return createStaticFileResponse(":/assets/client/" + filename, request);
		}
	}

    return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Requested asset was not found"));
}

HttpResponse ServerController::locateFileByType(const HttpRequest& request)
{
    // Check all parameters
    if (!request.getUrlParams().contains("ps_url_id"))
    {
        return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Sample id has not been provided"));
	}
	QString ps_url_id = request.getUrlParams()["ps_url_id"];
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

    FileLocationList file_list;
    QJsonDocument json_doc_output;
    QJsonDocument json_doc_without_tokens;
    QJsonArray json_list_output;
    QJsonArray json_list_without_tokens;
    PathType requested_type = FileLocation::stringToType(request.getUrlParams()["type"].toUpper().trimmed());

    QString locus;
    if (request.getUrlParams().contains("locus"))
    {
        locus = request.getUrlParams()["locus"];
    }

    // Start looking for FileLocations
    UrlEntity url_entity = UrlManager::getURLById(ps_url_id.trimmed());
    QString found_file = url_entity.filename_with_path;
    if (!url_entity.file_exists)
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Processed sample file does not exist"));
    }

    // Check the cache first
    ServerDB db = ServerDB();
    if (db.hasFileLocation(found_file, request.getUrlParams()["type"].toUpper().trimmed(), locus, multiple_files, return_if_missing))
    {
        QJsonArray updated_cached_array;
        QJsonDocument cache_doc = db.getFileLocation(found_file, request.getUrlParams()["type"].toUpper().trimmed(), locus, multiple_files, return_if_missing);
        db.updateFileLocation(found_file, request.getUrlParams()["type"].toUpper().trimmed(), locus, multiple_files, return_if_missing);
        QJsonArray cached_array = cache_doc.array();
        for (const QJsonValue &value : cached_array)
        {
            if (value.isObject())
            {
                QJsonObject cached_object = value.toObject();
                QString cached_filename = cached_object.value("filename").toString();
                if (!cached_filename.isEmpty())
                {
                    cached_object.insert("filename", createTempUrl(cached_filename, request.getUrlParams()["token"]));
                    updated_cached_array.append(cached_object);
                }
            }
        }

        // Ignore the cache entry, if no URLs were genereated
        if (updated_cached_array.size()>0)
        {
            QJsonDocument updated_cached_doc;
            updated_cached_doc.setArray(updated_cached_array);

            BasicResponseData response_data;
            response_data.length = updated_cached_doc.toJson().length();
            response_data.content_type = request.getContentType();
            response_data.is_downloadable = false;
            return HttpResponse(response_data, updated_cached_doc.toJson());
        }
    }

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
                if (locus.isEmpty())
                {
                    return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Locus value has not been provided"));
				}
                file_list << file_locator->getRepeatExpansionImage(locus);
				break;
			case PathType::BAM:
            case PathType::CRAM:
				file_list = file_locator->getBamFiles(return_if_missing);
				break;
			case PathType::VIRAL_BAM:
				file_list = file_locator->getViralBamFiles(return_if_missing);
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
            case PathType::REPEAT_EXPANSION_HISTOGRAM:
                if (locus.isEmpty())
                {
                    return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Locus value has not been provided"));
                }
                file_list << file_locator->getRepeatExpansionHistogram(locus);
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
			case PathType::SIGNATURE_SBS:
				file_list << file_locator->getSignatureSbsFile();
				break;
			case PathType::SIGNATURE_ID:
				file_list << file_locator->getSignatureIdFile();
				break;
			case PathType::SIGNATURE_DBS:
				file_list << file_locator->getSignatureDbsFile();
				break;
			case PathType::SIGNATURE_CNV:
				file_list << file_locator->getSignatureCnvFile();
                break;
            case PathType::CFDNA_CANDIDATES:
                file_list << file_locator->getSomaticCfdnaCandidateFile();
                break;            
			case PathType::METHYLATION:
				file_list << file_locator->getMethylationFile();
				break;
			case PathType::METHYLATION_IMAGE:
				if (locus.isEmpty())
				{
					return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Locus value has not been provided"));
				}
				file_list << file_locator->getMethylationImage(locus);
				break;
			case PathType::PARAPHASE_EVIDENCE:
				file_list = file_locator->getParaphaseEvidenceFiles(return_if_missing);
				break;
			case PathType::GSVAR:
				file_list << FileLocation(url_entity.file_id, PathType::GSVAR, found_file, true);
				break;
            case PathType::SAMPLE_FOLDER:
            case PathType::FUSIONS_PIC_DIR:
            case PathType::FUSIONS:
            case PathType::FUSIONS_BAM:
            case PathType::MANTA_FUSIONS:
            case PathType::COUNTS:
            case PathType::EXPRESSION_COHORT:
            case PathType::EXPRESSION_STATS:
            case PathType::EXPRESSION_CORR:
            case PathType::EXPRESSION_EXON:
            case PathType::SPLICING_BED:
            case PathType::SPLICING_ANN:
            case PathType::VIRAL:
            case PathType::VCF_CF_DNA:
            case PathType::MRD_CF_DNA:
            case PathType::HLA_GENOTYPER:
            case PathType::OTHER:
                return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "The type '" + request.getUrlParams()["type"].toUpper().trimmed() + "' cannot be handled by this endpoint"));
		}
	}


	for (int i = 0; i < file_list.count(); ++i)
	{
		Log::info("file path: " + file_list.at(i).filename);
        QJsonObject cur_json_item;
        QJsonObject cur_json_item_without_token;
		cur_json_item.insert("id", file_list[i].id);
        cur_json_item.insert("type", file_list[i].typeAsString());
        cur_json_item.insert("modified", file_list[i].modifiedAsString());
        cur_json_item.insert("exists", file_list[i].exists);

        cur_json_item_without_token = cur_json_item;

		bool needs_url = true;
		if (request.getUrlParams().contains("path"))
		{
			if (request.getUrlParams()["path"].toLower() == "absolute") needs_url = false;
		}
        if (needs_url)
		{
			try
            {
                cur_json_item.insert("filename", createTempUrl(file_list[i].filename, request.getUrlParams()["token"]));
            }
			catch (Exception& e)
            {
                return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
			}
		}
		else
		{
			cur_json_item.insert("filename", file_list[i].filename);            
		}

        cur_json_item_without_token.insert("filename", file_list[i].filename);
		json_list_output.append(cur_json_item);
        json_list_without_tokens.append(cur_json_item_without_token);
	}

	json_doc_output.setArray(json_list_output);
    json_doc_without_tokens.setArray(json_list_without_tokens);

    db.addFileLocation(found_file, request.getUrlParams()["type"].toUpper().trimmed(), locus, static_cast<int>(multiple_files), static_cast<int>(return_if_missing), json_doc_without_tokens.toJson(QJsonDocument::Compact), QDateTime::currentDateTime());

	BasicResponseData response_data;
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getProcessedSamplePath(const HttpRequest& request)
{
    PathType type;
    try
    {
        type = FileLocation::stringToType(request.getUrlParams()["type"].toUpper().trimmed());
    }
    catch (Exception& e)
    {
        return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

    int id = request.getUrlParams()["ps_id"].toInt();

    QString ps_name;
    QString found_file_path;
    try
    {
        ps_name = NGSD().processedSampleName(request.getUrlParams()["ps_id"]);
        found_file_path = getProcessedSampleFile(id, type, EndpointManager::getTokenIfAvailable(request));
    }
    catch (DatabaseException& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }
    catch (HttpException& e)
    {
        return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }
    catch (Exception& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

    FastFileInfo file_info(found_file_path);
    FileLocation project_file = FileLocation(ps_name, type, createTempUrl(file_info, request.getUrlParams()["token"]), file_info.lastModified(), file_info.exists());

	QJsonDocument json_doc_output;
	QJsonArray file_location_as_json_list;
	QJsonObject file_location_as_json_object;
	file_location_as_json_object.insert("id", ps_name);
	file_location_as_json_object.insert("type", project_file.typeAsString());
	file_location_as_json_object.insert("filename", project_file.filename);
    file_location_as_json_object.insert("modified", project_file.modifiedAsString());
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
    catch (DatabaseException& e)
    {
        Log::error("Database error while looking for the analysis job GSvar file in NGSD: " + e.message());
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }
	catch (Exception& e)
	{
        Log::error("Error while looking for the analysis job GSvar file in NGSD: " + e.message());
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
	}

    FastFileInfo file_info(found_file_path);
    FileLocation analysis_job_gsvar_file = FileLocation(ps_name, PathType::GSVAR, createTempUrl(file_info, request.getUrlParams()["token"]), file_info.exists());
	QJsonDocument json_doc_output;
	QJsonObject file_location_as_json_object;

	file_location_as_json_object.insert("id", ps_name);
	file_location_as_json_object.insert("type", analysis_job_gsvar_file.typeAsString());
	file_location_as_json_object.insert("filename", analysis_job_gsvar_file.filename);
    file_location_as_json_object.insert("modified", analysis_job_gsvar_file.modifiedAsString());
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
	QJsonDocument json_doc_output;
	QJsonObject last_update_as_json_object;
	FileInfo log_info;

	try
	{
		log_info = NGSD().analysisJobLatestLogInfo(job_id);
	}
    catch(DatabaseException& e)
    {
        Log::error("Database error while reading the analysis job latest log info: " + EndpointManager::formatResponseMessage(request, e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }
	catch(Exception& e)
    {
        Log::error("Error while reading the analysis job latest log info: " + EndpointManager::formatResponseMessage(request, e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
	}

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
	FileLocation analysis_job_log_file;

	try
    {
        NGSD db;
        int job_id = request.getUrlParams()["job_id"].toInt();
        AnalysisJob job = db.analysisInfo(job_id, true);
        QString id = db.processedSampleName(db.processedSampleId(job.samples[0].name));
        QString log = db.analysisJobLatestLogInfo(job_id).file_name_with_path;

        FastFileInfo file_info(log);
        analysis_job_log_file = FileLocation(id, PathType::OTHER, createTempUrl(file_info, request.getUrlParams()["token"]), file_info.exists());
	}
    catch (DatabaseException& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Database error while looking for the analysis job log file: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }
	catch (Exception& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Error while looking for the analysis job log file: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
	}

	QJsonDocument json_doc_output;
	QJsonObject file_location_as_json_object;

	file_location_as_json_object.insert("id", analysis_job_log_file.id);
	file_location_as_json_object.insert("type", analysis_job_log_file.typeAsString());
	file_location_as_json_object.insert("filename", analysis_job_log_file.filename);
    file_location_as_json_object.insert("modified", analysis_job_log_file.modifiedAsString());
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
        return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "The GSvar file in " + ps_url_id + "could not be located"));
	}

	QJsonDocument json_doc;
	try
	{
		json_doc = QJsonDocument::fromJson(request.getBody());
	}
	catch (Exception& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Error while parsing changes for the GSvar file" + url.filename_with_path + ":" + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Changes for the GSvar file in " + ps_url_id + "could not be parsed: " + e.message()));
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
                return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not identify key columns in GSvar file: " + ps_url_id));
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
                        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not identify changed column " + column + " in GSvar file: " + ps_url_id));
					}
					is_current_variant_changed = true;
					is_file_changed = true;

					line_columns[column_names.indexOf(column)] = QUrl::toPercentEncoding(text); // text.replace("\n", " ").replace("\t", " ");
				}
			}
			catch (Exception& e)
            {
                Log::error(EndpointManager::formatResponseMessage(request, "Error while processing changes for the GSvar file " + url.filename_with_path + ": " + e.message()));
                return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Changes for the GSvar file in " + ps_url_id + "could not be parsed: " + e.message()));
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
            Log::warn(EndpointManager::formatResponseMessage(request, "Could not remove: " + in_file.data()->fileName()));
		}
		//put the changed copy instead of the original
		if (!out_file.data()->rename(url.filename_with_path))
        {
            Log::warn(EndpointManager::formatResponseMessage(request, "Could not rename: " + out_file.data()->fileName()));
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
	Helper::mkdir(qbic_data_path);
	if (!qbic_data_path.endsWith(QDir::separator())) qbic_data_path = qbic_data_path + QDir::separator();

	QString filename = request.getUrlParams()["filename"];
	QString folder_name = request.getUrlParams()["id"];
	QString content = request.getBody();

	if ((filename.isEmpty()) || (folder_name.isEmpty()))
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Path or filename has not been provided"));
	}

	// It should not be possible to move up to the parent directory or to access system directories
	folder_name = folder_name.replace(".", "");
	folder_name = folder_name.replace(QDir::separator(), "");
	folder_name = qbic_data_path + folder_name;

	Helper::mkdir(folder_name);

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
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not save the data: " + e.message()));
	}

	return HttpResponse(ResponseStatus::OK, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), filename + " has been saved");
}

HttpResponse ServerController::uploadFile(const HttpRequest& request)
{	
    if (!request.getUrlParams().contains("ps_url_id"))
    {
        return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Processed sample id is missing."));
	}
    UrlEntity url_entity = UrlManager::getURLById(request.getUrlParams()["ps_url_id"].trimmed());

    return uploadFileToFolder(url_entity.path, request);
}

HttpResponse ServerController::annotateVariant(const HttpRequest& request)
{
    QString input_vcf = Helper::tempFileName(".vcf");
    Helper::storeTextFile(input_vcf, QStringList() << request.getBody());
    QString validation_result;
    QTextStream out_stream(&validation_result);
    if (!VcfFile::isValid(input_vcf, Settings::string("reference_genome", true), out_stream, false, std::numeric_limits<int>::max()))
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Invalid input VCF data: " + validation_result));
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QString an_vep_out = Helper::tempFileName(".vcf");
    Log::info("Running megSAP >> an_vep.php: " + an_vep_out);
	process.start("php", QStringList() << PipelineSettings::rootDir() + "/src/Tools/an_vep.php" << "-in" << input_vcf << "-out" << an_vep_out);
    bool success = process.waitForFinished(-1);
    Log::error("Exit code = " + QString::number(process.exitCode()));
    if (!success || process.exitCode()>0)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, QString("Error while executing an_vep.php: " + process.readAll())));
    }
    Log::info(process.readAll());

    QString vcf2gsvar_out = Helper::tempFileName(".GSvar");
    Log::info("Running megSAP >> vcf2gsvar.php: " + vcf2gsvar_out);
	process.start("php", QStringList() << PipelineSettings::rootDir() + "/src/Tools/vcf2gsvar.php" << "-in" << an_vep_out << "-out" << vcf2gsvar_out);
    success = process.waitForFinished(-1);
    if (!success || process.exitCode()>0)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, QString("Error while executing vcf2gsvar.php: " + process.readAll())));
    }
    Log::info(EndpointManager::formatResponseMessage(request, process.readAll()));

    return createStaticStreamResponse(vcf2gsvar_out, true);
}
HttpResponse ServerController::calculateLowCoverage(const HttpRequest& request)
{
	BedFile roi;
	QString bam_file_name;
	int cutoff = 0;

	if (request.getFormUrlEncoded().contains("roi"))
	{
		roi = roi.fromText(request.getFormUrlEncoded()["roi"].toUtf8());
	}
	if (request.getFormUrlEncoded().contains("bam_url_id"))
	{
		bam_file_name = UrlManager::getURLById(request.getFormUrlEncoded()["bam_url_id"]).filename_with_path;
	}

    FastFileInfo *info = new FastFileInfo(bam_file_name);
    if (!info->exists())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), EndpointManager::formatResponseMessage(request, "BAM file does not exist: " + bam_file_name));
    }

	if (request.getFormUrlEncoded().contains("cutoff"))
	{
		cutoff = request.getFormUrlEncoded()["cutoff"].toInt();
	}

	int threads = Settings::integer("threads");
	BedFile low_cov = Statistics::lowCoverage(roi, bam_file_name, cutoff, 1, 0, threads);

	QByteArray body = low_cov.toText().toUtf8();

	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, body);
}

HttpResponse ServerController::calculateAvgCoverage(const HttpRequest& request)
{
	BedFile roi;
	QString bam_file_name;

    if (request.getFormUrlEncoded().contains("roi"))
	{
        roi = roi.fromText(request.getFormUrlEncoded()["roi"].toUtf8());
	}
	if (request.getFormUrlEncoded().contains("bam_url_id"))
	{
		bam_file_name = UrlManager::getURLById(request.getFormUrlEncoded()["bam_url_id"]).filename_with_path;
	}

    FastFileInfo *info = new FastFileInfo(bam_file_name);
    if (!info->exists())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), EndpointManager::formatResponseMessage(request, "BAM file does not exist: " + bam_file_name));
    }

	int threads = Settings::integer("threads");
	Statistics::avgCoverage(roi, bam_file_name, 1, threads);

    QByteArray body = roi.toText().toUtf8();
    BasicResponseData response_data;
    response_data.length = body.length();
    response_data.content_type = request.getContentType();
    response_data.is_stream = (!body.isEmpty() ? true : false);
    response_data.is_downloadable = false;

    return HttpResponse(response_data, body);
}

HttpResponse ServerController::calculateTargetRegionReadDepth(const HttpRequest& request)
{
    BedFile roi;
	QString bam_file_name;

    if (request.getFormUrlEncoded().contains("roi"))
	{
        roi = roi.fromText(request.getFormUrlEncoded()["roi"].toUtf8());
	}
	if (request.getFormUrlEncoded().contains("bam_url_id"))
	{
		bam_file_name = UrlManager::getURLById(request.getFormUrlEncoded()["bam_url_id"]).filename_with_path;
	}

    FastFileInfo *info = new FastFileInfo(bam_file_name);
    if (!info->exists())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), EndpointManager::formatResponseMessage(request, "BAM file does not exist: " + bam_file_name));
    }

	QString ref_file = Settings::string("reference_genome");
    QCCollection stats = Statistics::mapping(roi, bam_file_name, ref_file);

	for (int i=0; i<stats.count(); ++i)
	{
		if (stats[i].accession()=="QC:2000025")
		{
			QByteArray body = stats[i].toString().toUtf8();
			BasicResponseData response_data;
			response_data.length = body.length();
			response_data.content_type = request.getContentType();
			response_data.is_downloadable = false;
			return HttpResponse(response_data, body);
		}
	}

    return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), EndpointManager::formatResponseMessage(request, "Could not calculate target region read depth"));
}

HttpResponse ServerController::getMultiSampleAnalysisInfo(const HttpRequest& request)
{
    if (!request.getFormUrlEncoded().contains("analyses"))
	{
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), EndpointManager::formatResponseMessage(request, "Could not get any information about the multi-sample analysis"));
    }

    QJsonDocument json_in_doc = QJsonDocument::fromJson(QUrl::fromPercentEncoding(request.getFormUrlEncoded()["analyses"].toUtf8()).toUtf8());
    if (!json_in_doc.isArray())
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), EndpointManager::formatResponseMessage(request, "Could not parse the server response into JSON"));
    }

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
            ps_sample_name_array.append(info.name);
            QString current_ps_id;
            try
            {
                current_ps_id = NGSD().processedSampleId(info.name);
            }
            catch (DatabaseException& e)
            {
                Log::error(EndpointManager::formatResponseMessage(request, "Database error while looking for the processed sample id for a multisample analysis: " + e.message()));
                return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
            }
            catch (Exception& e)
            {
                Log::error(EndpointManager::formatResponseMessage(request, "Error while looking for the processed sample id for a multisample analysis: " + e.message()));
                return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
            }

            ps_sample_id_array.append(current_ps_id);
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
HttpResponse ServerController::performLogin(const HttpRequest& request)
{
	if (!request.getFormUrlEncoded().contains("name") || !request.getFormUrlEncoded().contains("password"))
    {
        return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), EndpointManager::formatResponseMessage(request, "No username or/and password were found"));
	}

	QString user_name = request.getFormUrlEncoded()["name"];
	QString message;

	try
    {
        message = NGSD().checkPassword(user_name, request.getFormUrlEncoded()["password"]);
	}
    catch (DatabaseException& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Database error while checking the user password: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
	}
    catch (Exception& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Error while checking the user password: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

    if (!message.isEmpty())
	{
        return HttpResponse(ResponseStatus::UNAUTHORIZED, request.getContentType(), EndpointManager::formatResponseMessage(request, "Invalid username or password"));
    }

    QString secure_token = ServerHelper::generateUniqueStr();

    int user_id;
    try
    {
        user_id = NGSD().userId(user_name);
    }
    catch(DatabaseException& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Database error while getting user id: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }
    catch(Exception& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Error while getting user id: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

    QString user_login = "unknown";
    QString user_real_name = "unknown";

    try
    {
        NGSD db;
        user_login = db.userLogin(user_id);
        user_real_name = db.userName(user_id);
    }
    catch (DatabaseException& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Database request failed: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

    Session cur_session = Session(secure_token, user_id, user_login, user_real_name, QDateTime::currentDateTime(), false);
    SessionManager::addNewSession(cur_session);
    QByteArray body = secure_token.toUtf8();

    BasicResponseData response_data;
    response_data.length = body.length();
    response_data.content_type = request.getContentType();
    response_data.is_downloadable = false;
    Log::info(EndpointManager::formatResponseMessage(request, "User " + user_name + " has logged in"));
    return HttpResponse(response_data, body);
}

HttpResponse ServerController::getSessionInfo(const HttpRequest& request)
{
	QString token = EndpointManager::getTokenIfAvailable(request);
    if (token.isEmpty())
    {
        return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), EndpointManager::formatResponseMessage(request, "You are not allowed to access this information"));
    }
    Session current_session = SessionManager::getSessionBySecureToken(token);

    QJsonDocument json_doc;
    QJsonObject json_object;

    qint64 valid_period = 0;
    try
    {
        valid_period = Settings::integer("session_duration");
    }
    catch(ProgrammingException& e)
    {
        valid_period = SessionManager::DEFAULT_VALID_PERIOD;
        Log::warn(e.message() + " Using the default value: " + QString::number(valid_period));
    }

    json_object.insert("user_id", current_session.user_id);
    json_object.insert("login_time", current_session.login_time.toSecsSinceEpoch());
    json_object.insert("is_db_token", current_session.is_for_db_only);
    json_object.insert("valid_period", valid_period);
    json_doc.setObject(json_object);

    BasicResponseData response_data;
    response_data.length = json_doc.toJson().length();
    response_data.content_type = request.getContentType();
    response_data.is_downloadable = false;
    return HttpResponse(response_data, json_doc.toJson());
}

HttpResponse ServerController::validateCredentials(const HttpRequest& request)
{
	QString message;
	try
	{
		message = NGSD().checkPassword(request.getFormUrlEncoded()["name"], request.getFormUrlEncoded()["password"]);
	}
    catch (DatabaseException& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Database error while checking the user password: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
	}
    catch (Exception& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Error while checking the user password: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

	QByteArray body = message.toUtf8();
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
        return HttpResponse(ResponseStatus::UNAUTHORIZED, request.getContentType(), EndpointManager::formatResponseMessage(request, "You need to log in first"));
    }

    QString db_token = ServerHelper::generateUniqueStr();
    Session cur_session = Session(db_token, user_session.user_id, user_session.user_login, user_session.user_name, QDateTime::currentDateTime(), true);
    SessionManager::addNewSession(cur_session);
	QByteArray body = db_token.toUtf8();

	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;
	return HttpResponse(response_data, body);
}

HttpResponse ServerController::getNgsdCredentials(const HttpRequest& request)
{
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
        return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), EndpointManager::formatResponseMessage(request, "Secure token has not been provided"));
	}
    QString token = request.getFormUrlEncoded()["token"];

    if (!SessionManager::isValidSession(token))
	{
        return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), EndpointManager::formatResponseMessage(request, "You have provided an invalid token"));
    }

    Session current_session = SessionManager::getSessionBySecureToken(token);
    try
    {
        SessionManager::removeSession(token);
    }
    catch (Exception& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), EndpointManager::formatResponseMessage(request, e.message()));
    }
    body = request.getFormUrlEncoded()["token"].toUtf8();

    BasicResponseData response_data;
    response_data.length = body.length();
    response_data.content_type = request.getContentType();
    response_data.is_downloadable = false;

    try
    {
        if (!current_session.isEmpty()) Log::info(EndpointManager::formatResponseMessage(request, "User " + current_session.user_login + " (" + current_session.user_name + ") has logged out"));
    }
    catch (Exception& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, request.getContentType(), EndpointManager::formatResponseMessage(request, e.message()));
    }
    return HttpResponse(response_data, body);
}

HttpResponse ServerController::getProcessingSystemRegions(const HttpRequest& request)
{
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename;

	try
	{
		filename = NGSD().processingSystemRegionsFilePath(sys_id.toInt());
	}
    catch(DatabaseException& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Error while getting the processing system regions file path: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
	}
    catch(Exception& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "While getting the processing system regions file path: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

	if (filename.isEmpty())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Processing system regions file has not been found"));
	}
	return createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getProcessingSystemGenes(const HttpRequest& request)
{
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename;

	try
	{
		filename = NGSD().processingSystemGenesFilePath(sys_id.toInt());
	}
    catch(DatabaseException& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Database error while getting the processing system genes file path: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
	}
    catch(Exception& e)
    {
        Log::error(EndpointManager::formatResponseMessage(request, "Error while getting the processing system genes file path: " + e.message()));
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, e.message()));
    }

	if (filename.isEmpty())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Processing system genes file has not been found"));
	}
	return createStaticStreamResponse(filename, false);
}

HttpResponse ServerController::getSecondaryAnalyses(const HttpRequest& request)
{
	QString processed_sample_name = request.getUrlParams()["ps_name"];
	QString type  = QUrl::fromEncoded(request.getUrlParams()["type"].toUtf8()).toString();
	QStringList secondary_analyses;
	try
	{
		QStringList analyses = NGSD().secondaryAnalyses(processed_sample_name, type);
		foreach(QString file, analyses)
		{
            FastFileInfo *info = new FastFileInfo(file);
            if (info->exists())
			{
				secondary_analyses << file;
			}
		}
	}
	catch (DatabaseException& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Error while getting secondary analyses from the database: " + e.message()));
	}
    catch (Exception& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not get secondary analyses: " + e.message()));
    }

	QJsonDocument json_doc_output;
	QJsonArray json_array;
	for (int i = 0; i < secondary_analyses.count(); i++)
	{
		json_array.append(createTempUrl(secondary_analyses[i], request.getUrlParams()["token"]));
	}
	json_doc_output.setArray(json_array);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getRnaFusionPics(const HttpRequest& request)
{
	QString rna_id = request.getUrlParams()["rna_id"];
	QString ps_id;
	QString filename;

	try
	{
        NGSD db;
        ps_id = db.processedSampleId(rna_id);
		if (ps_id.isEmpty())
        {
            return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not find a processed sample id " + rna_id));
		}
        filename = db.processedSamplePath(ps_id, PathType::FUSIONS_PIC_DIR);
	}
    catch(DatabaseException& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Error while getting secondary analyses from the database: " + e.message()));
    }
	catch(Exception& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not get secondary analyses: " + e.message()));
	}

	if (filename.isEmpty())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not find a processed sample path for " + ps_id));
	}

	QStringList found_files = Helper::findFiles(filename, "*.png", false);

	QJsonDocument json_doc_output;
	QJsonArray json_array;
	for (int i = 0; i < found_files.count(); i++)
	{
		json_array.append(createTempUrl(found_files[i], request.getUrlParams()["token"]));
	}
	json_doc_output.setArray(json_array);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getRnaExpressionPlots(const HttpRequest& request)
{
	QString rna_id = request.getUrlParams()["rna_id"];
	QString ps_id;
	QString filename;

	try
	{
        NGSD db;
        ps_id = db.processedSampleId(rna_id);
		if (ps_id.isEmpty())
        {
            return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not find a processed sample id " + rna_id));
		}
        filename = db.processedSamplePath(ps_id, PathType::SAMPLE_FOLDER);
	}
    catch(DatabaseException& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Error while getting secondary analyses from the database: " + e.message()));
    }
	catch(Exception& e)
    {
        return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not get secondary analyses: " + e.message()));
	}

	if (filename.isEmpty())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), EndpointManager::formatResponseMessage(request, "Could not find a processed sample path for " + ps_id));
	}

	QStringList found_files = Helper::findFiles(filename, rna_id + "_expr.*.png", false);

	QJsonDocument json_doc_output;
	QJsonArray json_array;
	for (int i = 0; i < found_files.count(); i++)
	{
		json_array.append(createTempUrl(found_files[i], request.getUrlParams()["token"]));
	}
	json_doc_output.setArray(json_array);

	BasicResponseData response_data;
	response_data.byte_ranges = QList<ByteRange>{};
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = request.getContentType();
	response_data.is_downloadable = false;

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getCurrentClientInfo(const HttpRequest& /*request*/)
{
	QJsonDocument json_doc_output;
	QJsonObject json_object;

	json_object.insert("version", SessionManager::getCurrentClientInfo().version);
	json_object.insert("message", SessionManager::getCurrentClientInfo().message);
	json_object.insert("date", SessionManager::getCurrentClientInfo().date.toSecsSinceEpoch());
	json_doc_output.setObject(json_object);

	BasicResponseData response_data;
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = ContentType::APPLICATION_JSON;
	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse ServerController::getCurrentNotification(const HttpRequest& /*request*/)
{
	QJsonDocument json_doc_output;
	QJsonObject json_object;

	json_object.insert("id", SessionManager::getCurrentNotification().id);
	json_object.insert("message", SessionManager::getCurrentNotification().message);
	json_doc_output.setObject(json_object);

	BasicResponseData response_data;
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = ContentType::APPLICATION_JSON;
	return HttpResponse(response_data, json_doc_output.toJson());
}

QString ServerController::findPathForTempUrl(QList<QString> path_parts)
{
	if (!path_parts.isEmpty())
	{
		UrlEntity url_entity = UrlManager::getURLById(path_parts[0]);
		if (!url_entity.filename_with_path.isEmpty())
		{
			path_parts.removeAt(0);
            QString path = url_entity.path;
            if (!url_entity.path.endsWith(QDir::separator())) path += QDir::separator();
            return path + path_parts.join(QDir::separator());
		}
	}

	return "";
}

QString ServerController::findPathForServerFolder(const QList<QString>& path_parts, QString server_folder)
{
	if (!server_folder.endsWith(QDir::separator()))
	{
		server_folder = server_folder + QDir::separator();
	}
	QString served_file = server_folder.trimmed() + path_parts.join(QDir::separator());

	served_file = QUrl::fromEncoded(served_file.toUtf8()).toString(); // handling browser endcoding, e.g. spaces and other characters in names
	int param_pos = served_file.indexOf("?");
	if (param_pos > -1) served_file = served_file.left(param_pos);
	if (QFile(served_file).exists()) return served_file;

	return "";
}

bool ServerController::hasOverlappingRanges(const QList<ByteRange>& ranges)
{
	if (ranges.count() == 1) return false;
	for (int i = 0; i < ranges.count(); ++i)
	{
		for (int r = 0; r < ranges.count(); ++r)
		{
			if (i == r) continue;

			// one range contains another
			if ((ranges[i].start>=ranges[r].start) && (ranges[i].end<=ranges[r].end)) return true;

			// ranges partly overlap
			if ((ranges[i].start<ranges[r].start) && (ranges[i].end>ranges[r].start)) return true;
			if ((ranges[i].start==ranges[r].end) || (ranges[i].end==ranges[r].start)) return true;
		}
	}

	return false;
}

QString ServerController::getProcessedSampleFile(const int& ps_id, const PathType& type, const QString& token)
{
    QString found_file_path;
    try
    {
        Session current_session = SessionManager::getSessionBySecureToken(token);

        NGSD db;
        // access is restricted only for the user role 'user_restricted'
        QString role = db.getUserRole(current_session.user_id);
        if (role=="user_restricted" && !db.userCanAccess(current_session.user_id, ps_id))
        {
            THROW_HTTP(HttpException, "You do not have permissions to the sample with id '" + QString::number(ps_id) + "'", 401,  {}, {});
        }
        found_file_path = db.processedSamplePath(QString::number(ps_id), type);
    }
    catch (DatabaseException& e)
    {
        Log::error("Database error while opening processed sample from NGSD: " + e.message());
        THROW_HTTP(HttpException, e.message(), 500,  {}, {});
    }
    catch (Exception& e)
    {
        Log::error("Error opening processed sample from NGSD: " + e.message());
        THROW_HTTP(HttpException, e.message(), 500,  {}, {});
    }
    return found_file_path;
}

QString ServerController::createTempUrl(const QString& file, const QString& token)
{
    QString id = ServerHelper::generateUniqueStr();
    FastFileInfo *info = new FastFileInfo(file);
    UrlManager::addNewUrl(UrlEntity(id, info->fileName(), info->absolutePath(), file, id, info->size(), info->exists(), QDateTime::currentDateTime()));
    return ClientHelper::serverApiUrl() + "temp/" + id + "/" + info->fileName() + "?token=" + token;
}

QString ServerController::createTempUrl(FastFileInfo& file_info, const QString& token)
{
    QString id = ServerHelper::generateUniqueStr();
    UrlManager::addNewUrl(UrlEntity(id, file_info.fileName(), file_info.absolutePath(), file_info.absoluteFilePath(), id, file_info.size(), file_info.exists(), QDateTime::currentDateTime()));
    return ClientHelper::serverApiUrl() + "temp/" + id + "/" + file_info.fileName() + "?token=" + token;
}

QString ServerController::stripParamsFromTempUrl(const QString& url)
{
    QString output = url;
    int param_start_pos = url.indexOf("?");

    if (param_start_pos != -1)
    {
        output = url.left(param_start_pos);
    }

    return output;
}

HttpResponse ServerController::createStaticFolderResponse(const QString path, const HttpRequest& request)
{
	if (!Settings::boolean("allow_folder_listing", true))
    {
        return HttpResponse(ResponseStatus::FORBIDDEN, ContentType::TEXT_HTML, EndpointManager::formatResponseMessage(request, "Requested location is not available due to the access restrictions"));
	}

	QDir dir(path);
	if (!dir.exists())
    {
        return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, EndpointManager::formatResponseMessage(request, "Requested folder does not exist"));
	}

	QString base_folder_url = ClientHelper::serverApiUrl() + request.getPath();
	if (!base_folder_url.endsWith("/"))
	{
		base_folder_url = base_folder_url + "/";
	}
	QString cur_folder_url = base_folder_url + request.getPathItems().join("/");
	if (!cur_folder_url.endsWith("/"))
	{
		cur_folder_url = cur_folder_url + "/";
	}
	if (request.getPathItems().size()>0)
	{
		request.getPathItems().removeAt(request.getPathItems().size()-1);
	}
	QString parent_folder_url = base_folder_url + request.getPathItems().join("/");

	dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks);
	QFileInfoList list = dir.entryInfoList();
	QList<FolderItem> files {};
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		if ((fileInfo.fileName() == ".") || (fileInfo.fileName() == "..")) continue;

		FolderItem current_item;
		current_item.name = fileInfo.fileName();
		current_item.size = fileInfo.size();
		current_item.modified = fileInfo.lastModified();
		current_item.is_folder = fileInfo.isDir() ? true : false;
		files.append(current_item);
	}
	QString token = "";
	if (!request.getFormUrlEncoded().contains("token")) token = request.getUrlParams()["token"];

	QString output;
	QTextStream stream(&output);
	stream << HtmlEngine::getPageHeader("Folder content: " + dir.dirName());
	stream << HtmlEngine::getFolderIcons();
	stream << HtmlEngine::createFolderListingHeader(dir.dirName(), parent_folder_url);
	stream << HtmlEngine::createFolderListingElements(files, cur_folder_url, token);
	stream << HtmlEngine::getPageFooter();

	BasicResponseData response_data;
	response_data.length = output.toUtf8().length();
	response_data.is_stream = false;
	response_data.content_type = ContentType::TEXT_HTML;

	return HttpResponse(response_data, output.toUtf8());
}

HttpResponse ServerController::createStaticLocationResponse(const QString path, const HttpRequest& request)
{
    Log::info(EndpointManager::formatResponseMessage(request, "Accessing " + path));
	if ((!path.isEmpty()) && (QFileInfo(path).isDir()))
	{
		return createStaticFolderResponse(path, request);
	}

	return createStaticFileResponse(path, request);
}

HttpResponse ServerController::uploadFileToFolder(QString upload_folder, const HttpRequest& request)
{
	QStringList error_messages;

	if (upload_folder.isEmpty()) error_messages.append("Upload folder is missing");
    if (request.getMultipartFileName().isEmpty()) error_messages.append("Filename is missing");
    if (error_messages.size()>0)
	{
        return HttpResponse(ResponseStatus::BAD_REQUEST, ContentType::TEXT_PLAIN, EndpointManager::formatResponseMessage(request, "Parameters are missing: " + error_messages.join(" ")));
    }

    if (!upload_folder.endsWith("/")) upload_folder = upload_folder + "/";
    if (QDir(upload_folder).exists())
    {
        QSharedPointer<QFile> outfile = Helper::openFileForWriting(upload_folder + request.getMultipartFileName());
        outfile->write(request.getMultipartFileContent());
        return HttpResponse(ResponseStatus::OK, ContentType::TEXT_PLAIN, upload_folder + request.getMultipartFileName());
    }

    Log::error(EndpointManager::formatResponseMessage(request, "Upload folder does not exist: " + upload_folder));
    return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_PLAIN, EndpointManager::formatResponseMessage(request, "Upload destination does not exist on the server"));
}
