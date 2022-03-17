#include "EndpointController.h"

HttpResponse EndpointController::serveEndpointHelp(const HttpRequest& request)
{
	QByteArray body;
	if (request.getPathParams().count() == 2)
	{
		// Locate endpoint by URL and request method
		body = generateHelpPage(request.getPathParams()[0], HttpProcessor::getMethodTypeFromString(request.getPathParams()[1])).toLocal8Bit();
	}
	else if (request.getPathParams().count() == 1)
	{
		// For the same URL several request methods may be used: e.g. GET and POST
		body = generateHelpPage(request.getPathParams()[0]).toLocal8Bit();
	}
	else
	{
		// Help for all defined endpoints
		body = generateHelpPage().toLocal8Bit();
	}

	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = ContentType::TEXT_HTML;
	response_data.is_downloadable = false;
	return HttpResponse(response_data, body);
}

HttpResponse EndpointController::serveStaticFromServerRoot(const HttpRequest& request)
{
	if (!isAuthorizedWithToken(request))
	{
		return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are not authorized");
	}

	QString served_file = getServedRootPath(request.getPathParams());

	if (served_file.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), "Location has not been found in the server root folder");
	}

	if (QFileInfo(served_file).isDir())
	{
		return serveFolderContent(served_file, request.getPrefix(), request.getPath(), request.getPathParams());
	}

	return serveStaticFile(served_file, request.getMethod(), request.getContentType(), request.getHeaders());
}

HttpResponse EndpointController::serveStaticForTempUrl(const HttpRequest& request)
{
//	if (!isAuthorizedWithToken(request))
//	{
//		return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are not authorized");
//	}

	QString full_entity_path = getServedTempPath(request.getPathParams());

	if ((!full_entity_path.isEmpty()) && (QFileInfo(full_entity_path).isDir()))
	{
		return serveFolderContent(full_entity_path, request.getPrefix(), request.getPath(), request.getPathParams());
	}

	return serveStaticFile(full_entity_path, request.getMethod(), HttpProcessor::getContentTypeByFilename(full_entity_path), request.getHeaders());
}

HttpResponse EndpointController::serveStaticFileFromCache(const HttpRequest& request)
{
	QString filename = FileCache::getFileById(request.getPathParams()[0]).filename_with_path;
	return createStaticFromCacheResponse(filename, QList<ByteRange>{}, HttpProcessor::getContentTypeByFilename(filename), false);
}

HttpResponse EndpointController::createStaticFileRangeResponse(QString filename, QList<ByteRange> byte_ranges, ContentType type, bool is_downloadable)
{
	quint64 total_length = 0;
	for (int i = 0; i < byte_ranges.count(); ++i)
	{
		total_length = total_length + byte_ranges[i].length;
	}

	BasicResponseData response_data;
	response_data.filename = filename;
	response_data.length = total_length;
	response_data.byte_ranges = byte_ranges;
	response_data.file_size = QFile(filename).size();
	response_data.is_stream = true;
	response_data.content_type = type;
	response_data.is_downloadable = is_downloadable;

	return HttpResponse(response_data);
}

HttpResponse EndpointController::createStaticStreamResponse(QString filename, bool is_downloadable)
{
	BasicResponseData response_data;
	response_data.length = QFileInfo(filename).size();
	response_data.filename = filename;
	response_data.file_size = QFileInfo(filename).size();
	response_data.is_stream = true;
	response_data.content_type = HttpProcessor::getContentTypeByFilename(filename);
	response_data.is_downloadable = is_downloadable;

	return HttpResponse(response_data);
}

HttpResponse EndpointController::createStaticFromCacheResponse(QString id, QList<ByteRange> byte_ranges, ContentType type, bool is_downloadable)
{
	StaticFile static_file = FileCache::getFileById(id);

	if (static_file.content.isEmpty() || static_file.content.isNull())
	{
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Empty or corrpupted file");
	}

	BasicResponseData response_data;
	response_data.filename = FileCache::getFileById(id).filename_with_path;
	response_data.length = static_file.content.length();
	response_data.byte_ranges = byte_ranges;
	response_data.file_size = static_file.size;
	response_data.content_type = type;
	response_data.is_downloadable = is_downloadable;

	return HttpResponse(response_data, static_file.content);
}

EndpointController::EndpointController()
{
}

EndpointController& EndpointController::instance()
{
	static EndpointController endpoint_controller;
	return endpoint_controller;
}

HttpResponse EndpointController::serveFolderContent(QString path, QString request_prefix, QString request_path, QList<QString> request_path_params)
{
	if (!Settings::boolean("allow_folder_listing", true))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, ContentType::TEXT_HTML, "Requested location is not available due to the access restrictions");
	}

	QDir dir(path);
	if (!dir.exists())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, "Requested folder does not exist");
	}

	QString base_folder_url = ServerHelper::getUrlProtocol(false) + ServerHelper::getStringSettingsValue("server_host") + ":" + QString::number(ServerHelper::getNumSettingsValue("https_server_port")) + "/" + request_prefix + "/" + request_path;
	if (!base_folder_url.endsWith("/"))
	{
		base_folder_url = base_folder_url + "/";
	}
	QString cur_folder_url = base_folder_url + request_path_params.join("/");
	if (!cur_folder_url.endsWith("/"))
	{
		cur_folder_url = cur_folder_url + "/";
	}
	if (request_path_params.size()>0)
	{
		request_path_params.removeAt(request_path_params.size()-1);
	}
	QString parent_folder_url = base_folder_url + request_path_params.join("/");

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
	return serveFolderListing(dir.dirName(), cur_folder_url, parent_folder_url, files);
}

HttpResponse EndpointController::serveStaticFile(QString filename, RequestMethod method, ContentType content_type, QMap<QString, QList<QString>> headers)
{
	if ((filename.isEmpty()) || ((!filename.isEmpty()) && (!QFile::exists(filename))))
	{
		// Special case, when sending HEAD request for a file that does not exist
		if (method == RequestMethod::HEAD)
		{
			return HttpResponse(ResponseStatus::NOT_FOUND, content_type, 0.0);
		}

		return HttpResponse(ResponseStatus::NOT_FOUND, content_type, "Requested could not be found");
	}

	quint64 file_size = QFileInfo(filename).size();

	// Client wants to see only the size of the requested file (not its content)
	if (method == RequestMethod::HEAD)
	{
		return HttpResponse(ResponseStatus::OK, HttpProcessor::getContentTypeByFilename(filename), file_size);
	}

	// Random read functionality based on byte-range headers
	if (headers.contains("range"))
	{
		QList<ByteRange> byte_ranges;
		for (int i = 0; i < headers["range"].count(); ++i)
		{
			ByteRange current_range;
			current_range.start = 0;
			current_range.end = 0;

			QString range_value = headers["range"][i];
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
					if (current_range.end<=file_size)
					{
						current_range.start = file_size - current_range.end;
						current_range.end = file_size;
					}
				}
				if ((!is_end_set) && (is_start_set))
				{
					qDebug() << "Random read: offset end has been set as the end of file";
					current_range.end = file_size;
				}

				if ((!is_start_set) && (!is_end_set))
				{
					return HttpResponse(ResponseStatus::RANGE_NOT_SATISFIABLE, content_type, "Range limits have not been specified");
				}

				if (current_range.start > current_range.end)
				{
					return HttpResponse(ResponseStatus::RANGE_NOT_SATISFIABLE, content_type, "The requested range start position is greater than its end position");
				}
			}

			current_range.length = ((current_range.end - current_range.start) > 0) ? (current_range.end - current_range.start) : 0;
			current_range.length = current_range.length + 1;
			byte_ranges.append(current_range);
		}
		if (hasOverlappingRanges(byte_ranges))
		{
			return HttpResponse(ResponseStatus::RANGE_NOT_SATISFIABLE, content_type, "Overlapping ranges have been detected");
		}

		return createStaticFileRangeResponse(filename, byte_ranges, HttpProcessor::getContentTypeByFilename(filename), false);
	}


	// Stream the content of the entire file
	return createStaticStreamResponse(filename, false);
}

HttpResponse EndpointController::serveFolderListing(QString folder_title, QString cur_folder_url, QString parent_folder_url, QList<FolderItem> items)
{
	HttpResponse response;

	QString output;
	QTextStream stream(&output);
	stream << HtmlEngine::getPageHeader("Folder content: " + folder_title);
	stream << HtmlEngine::getFolderIcons();
	stream << HtmlEngine::createFolderListingHeader(folder_title, parent_folder_url);
	stream << HtmlEngine::createFolderListingElements(items, cur_folder_url);
	stream << HtmlEngine::getPageFooter();

	BasicResponseData response_data;
	response_data.length = output.toLocal8Bit().length();
	response_data.is_stream = false;
	response_data.content_type = ContentType::TEXT_HTML;

	return HttpResponse(response_data, output.toLocal8Bit());
}

QString EndpointController::getEndpointHelpTemplate(QList<Endpoint> endpoint_list)
{
	QString output;
	QTextStream stream(&output);

	stream << HtmlEngine::getPageHeader("API Help Page");
	stream << HtmlEngine::getApiHelpHeader("API Help Page");

	for (int i = 0; i < endpoint_list.count(); ++i)
	{
		QList<QString> param_names {};
		QList<QString> param_desc {};

		QMapIterator<QString, ParamProps> p(endpoint_list[i].params);
		while (p.hasNext()) {
			p.next();
			param_names.append(p.key());
			param_desc.append(p.value().comment);
		}

		HttpRequest request;
		request.setMethod(endpoint_list[i].method);

		stream << HtmlEngine::getApiHelpEntry(
					  endpoint_list[i].url,
					  request.methodAsString(),
					  param_names,
					  param_desc,
					  endpoint_list[i].comment
					);
	}

	stream << HtmlEngine::getPageFooter();

	return output;
}

QString EndpointController::generateHelpPage()
{
	return getEndpointHelpTemplate(EndpointManager::getEndpointEntities());
}

QString EndpointController::generateHelpPage(const QString& path, const RequestMethod& method)
{
	QList<Endpoint> selected_endpoints;
	selected_endpoints.append(EndpointManager::getEndpointByUrlAndMethod(path, method));
	return getEndpointHelpTemplate(selected_endpoints);
}

QString EndpointController::generateHelpPage(const QString& path)
{
	return getEndpointHelpTemplate(EndpointManager::getEndpointsByUrl(path));
}


QString EndpointController::getServedTempPath(QList<QString> path_parts)
{
	QString full_entity_path = "";
	if (path_parts.size() < 1)
	{
		return full_entity_path;
	}

	UrlEntity url_entity = UrlManager::getURLById(path_parts[0]);
	if (!url_entity.filename_with_path.isEmpty())
	{
		path_parts.removeAt(0);
		full_entity_path = QFileInfo(url_entity.filename_with_path).absolutePath() + QDir::separator() + path_parts.join(QDir::separator());
	}

	return full_entity_path;
}

QString EndpointController::getServedRootPath(const QList<QString>& path_parts)
{
	QString server_root = ServerHelper::getStringSettingsValue("server_root");
	if (!server_root.endsWith(QDir::separator()))
	{
		server_root = server_root + QDir::separator();
	}
	QString served_file = server_root.trimmed() + path_parts.join(QDir::separator());

	served_file = QUrl::fromEncoded(served_file.toLocal8Bit()).toString(); // handling browser endcoding, e.g. spaces and other characters in names
	int param_pos = served_file.indexOf("?");
	if (param_pos > -1) served_file = served_file.left(param_pos);

	if (QFile(served_file).exists())
	{
		return served_file;
	}

	return "";
}

StaticFile EndpointController::readFileContent(const QString& filename, const QList<ByteRange>& byte_ranges)
{
	StaticFile static_file {};
	static_file.filename_with_path = filename;
	static_file.modified = QFileInfo(filename).lastModified();

	QString found_id = FileCache::getFileIdIfInCache(filename);
	if (found_id.length() > 0)
	{
		return FileCache::getFileById(found_id);
	}

	QFile file(filename);
	static_file.size = file.size();
	if (!file.open(QIODevice::ReadOnly))
	{
		THROW(FileAccessException, "File could not be found: " + filename);
	}

	if ((!file.atEnd()) && (byte_ranges.length() == 0))
	{
		try
		{
			static_file.content = file.readAll();
		}
		catch (FileAccessException& e)
		{
			THROW(FileAccessException, "File cannot be processed (possibly due to the large size)");
		}
	}

	for (int i = 0; i < byte_ranges.count(); ++i)
	{
		if ((!file.atEnd()) && (byte_ranges[i].length > 0) && (file.seek(byte_ranges[i].start)))
		{
			static_file.content.append(file.read(byte_ranges[i].length));
		}
	}

	if ((!static_file.content.isEmpty()) && (Settings::boolean("static_cache", true)))
	{
		try
		{
			FileCache::addFileToCache(ServerHelper::generateUniqueStr(), filename, static_file.content, static_file.content.size());
		}
		catch (Exception& e)
		{
			qDebug() << "Could not add " << filename << " to the cache:" << e.message();
		}
	}

	file.close();

	return static_file;
}

QString EndpointController::addFileToCache(const QString& filename)
{
	readFileContent(filename, QList<ByteRange>{});
	return FileCache::getFileIdIfInCache(filename);
}

bool EndpointController::hasOverlappingRanges(const QList<ByteRange> ranges)
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

bool EndpointController::isAuthorizedWithToken(const HttpRequest& request)
{
	if (request.getUrlParams().contains("token"))
	{
		qDebug() << "Token" << SessionManager::isTokenValid(request.getUrlParams()["token"]);
		return SessionManager::isTokenValid(request.getUrlParams()["token"]);
	}

	return false;
}

