#include "EndpointController.h"

HttpResponse EndpointController::serveFolderContent(HttpRequest request)
{
	QString folder;
	if (request.getPathParams().count() == 0)
	{
		folder = Settings::string("server_root");
	}
	else
	{
		folder = request.getPathParams().value(0);
	}

	QDir dir(folder);
	if (!dir.exists())
	{
		return HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Requested folder does not exist"});
	}

	dir.setFilter(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);

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
//		qDebug() << "File:" + fileInfo.fileName() + ", " + fileInfo.size() + fileInfo.isDir();
	}
	return serveFolderListing(files);
}

HttpResponse EndpointController::serveEndpointHelp(HttpRequest request)
{
	QByteArray body;
	if (request.getPathParams().count() == 0)
	{
		body = generateGlobalHelp().toLocal8Bit();
	}
	else
	{
		body = generateEntityHelp(request.getPathParams().value(0), request.getMethod()).toLocal8Bit();
	}
	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = ContentType::TEXT_HTML;
	response_data.is_downloadable = false;
	return HttpResponse{false, false, "", HttpProcessor::generateHeaders(response_data), body};
}

HttpResponse EndpointController::serveStaticFile(HttpRequest request)
{
	QString served_file = getServedFileLocation(request.getPathParams().value(0));
	if (served_file.isEmpty())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, ContentType::TEXT_HTML, "File does not exist: " + request.getPathParams()[0]});
	}

	ByteRange byte_range {};
	byte_range.start = 0;
	byte_range.end = 0;
	if (request.getHeaders().contains("range"))
	{
		QString range_value = request.getHeaders().value("range");
		qDebug() << "Reading byte range header:" + range_value;
		range_value = range_value.replace("bytes", "");
		range_value = range_value.replace("=", "");
		range_value = range_value.trimmed();
		if (range_value.count("-") == 1)
		{
			byte_range.start = static_cast<quint64>(range_value.mid(0, range_value.indexOf("-")).trimmed().toULongLong());
			byte_range.end = static_cast<quint64>(range_value.mid(range_value.indexOf("-")+1, range_value.length()-range_value.indexOf("-")).trimmed().toULongLong());
		}
	}
	byte_range.length = ((byte_range.end - byte_range.start) > -1.0) ? (byte_range.end - byte_range.start) : 0;

	if (!request.getHeaders().contains("range"))
	{
		return createStaticStreamResponse(served_file, false);
	}
	return createStaticFileResponse(served_file, byte_range, HttpProcessor::getContentTypeByFilename(served_file), false);
}

HttpResponse EndpointController::serveStaticFileFromCache(HttpRequest request)
{
	QString filename = FileCache::getFileById(request.getPathParams().value(0)).filename_with_path;
	return createStaticFromCacheResponse(filename, ByteRange{}, HttpProcessor::getContentTypeByFilename(filename), false);
}

HttpResponse EndpointController::streamStaticFile(HttpRequest request)
{
	QString served_file = getServedFileLocation(request.getPathParams().value(0));
	if (served_file.isEmpty())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, ContentType::TEXT_HTML, "File does not exist: " + request.getPathParams()[0]});
	}

	bool downloadable = false;
	if (request.getUrlParams().size() > 0) downloadable = true;
	return createStaticStreamResponse(served_file, downloadable);
}

HttpResponse EndpointController::serveProtectedStaticFile(HttpRequest request)
{
	if (!isEligibileToAccess(request))
	{
		return HttpResponse(HttpError{StatusCode::FORBIDDEN, request.getContentType(), "Secure token has not been provided"});
	}
	// this is just an example
	return createStaticFileResponse(":/assets/client/example.png", ByteRange{}, ContentType::APPLICATION_OCTET_STREAM, true);
}

HttpResponse EndpointController::getFileInfo(HttpRequest request)
{
	QString filename = request.getUrlParams()["file"];
	if (request.getUrlParams()["file"].toLower().startsWith("http"))
	{
		QList<QString> url_parts = request.getUrlParams()["file"].split("/");
		if (url_parts.size() > 0)
		{
			UrlEntity current_entity = UrlManager::getURLById(url_parts[url_parts.size()-1]);
			if (current_entity.filename_with_path.length()>0)
			{
				filename = current_entity.filename_with_path;
			}
		}
	}

	if (!QFile(filename).exists())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, request.getContentType(), "File does not exist: " + filename});
	}

	QJsonDocument json_doc_output {};
	QJsonObject json_object {};
	json_object.insert("absolute_path", QFileInfo(filename).absolutePath());
	json_object.insert("base_name", QFileInfo(filename).baseName());
	json_object.insert("file_name", QFileInfo(filename).fileName());
	json_object.insert("last_modified", QString::number(QFileInfo(filename).lastModified().toSecsSinceEpoch()));
	json_object.insert("exists", QFileInfo(filename).exists());

	json_doc_output.setObject(json_object);

	BasicResponseData response_data;
	response_data.length = json_doc_output.toJson().length();
	response_data.content_type = ContentType::APPLICATION_JSON;
	response_data.is_downloadable = false;

	return HttpResponse{false, false, "", HttpProcessor::generateHeaders(response_data), json_doc_output.toJson()};
}

HttpResponse EndpointController::createStaticFileResponse(QString filename, ByteRange byte_range, ContentType type, bool is_downloadable)
{
	StaticFile static_file {};
	try
	{
		static_file = readFileContent(filename, byte_range);
	}
	catch(Exception& e)
	{
		return HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, e.message()});
	}

	BasicResponseData response_data;
	response_data.filename = getFileNameWithExtension(filename);
	response_data.length = static_file.content.length();
	response_data.byte_range = byte_range;
	response_data.file_size = static_file.size;
	response_data.content_type = type;
	response_data.is_downloadable = is_downloadable;

	return HttpResponse(false, false, "", HttpProcessor::generateHeaders(response_data), static_file.content);
}

HttpResponse EndpointController::createStaticStreamResponse(QString filename, bool is_downloadable)
{
	ContentType content_type = HttpProcessor::getContentTypeByFilename(filename);
	HttpResponse response;
	response.setIsBinary(false);
	if ((content_type == APPLICATION_OCTET_STREAM) || (content_type == IMAGE_PNG) || (content_type == IMAGE_JPEG))
	{
		response.setIsBinary(true);
	}

	response.setIsStream(true);
	response.setFilename(filename);
	response.addHeader("HTTP/1.1 200 OK\r\n");
	response.addHeader("Date: " + QDateTime::currentDateTime().toUTC().toString() + "\r\n");
	response.addHeader("Server: " + ServerHelper::getAppName() + "\r\n");
	response.addHeader("Transfer-Encoding: chunked\r\n");
	response.addHeader("Connection: Keep-Alive\r\n");
	response.addHeader("Content-Type: " + HttpProcessor::convertContentTypeToString(content_type) + "\r\n");

	if (is_downloadable)
	{
		response.addHeader("Content-Disposition: attachment; filename="+getFileNameWithExtension(filename)+"\r\n");
	}
	response.addHeader("\r\n");

	return response;
}

HttpResponse EndpointController::createStaticFromCacheResponse(QString id, ByteRange byte_range, ContentType type, bool is_downloadable)
{
	StaticFile static_file = FileCache::getFileById(id);

	if (static_file.content.isEmpty() || static_file.content.isNull())
	{
		return HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Empty or corrpupted file"});
	}

	BasicResponseData response_data;
	response_data.filename = getFileNameWithExtension(FileCache::getFileById(id).filename_with_path);
	response_data.length = static_file.content.length();
	response_data.byte_range = byte_range;
	response_data.file_size = static_file.size;
	response_data.content_type = type;
	response_data.is_downloadable = is_downloadable;

	return HttpResponse(false, false, "", HttpProcessor::generateHeaders(response_data), static_file.content);
}

EndpointController::EndpointController()
{
}

EndpointController& EndpointController::instance()
{
	static EndpointController endpoint_controller;
	return endpoint_controller;
}

HttpResponse EndpointController::serveFolderListing(QList<FolderItem> items)
{
	HttpResponse response;

	QString output;
	QTextStream stream(&output);
	stream << HtmlEngine::getPageHeader();
	stream << HtmlEngine::getFolderIcons();
	stream << HtmlEngine::createFolderListingHeader("Folder name", "");
	stream << HtmlEngine::createFolderListingElements(items);
	stream << HtmlEngine::getPageFooter();

	response.addHeader("HTTP/1.1 200 OK\n");
	response.addHeader("Content-Length: " + QString::number(output.toLocal8Bit().length()) + "\n");
	response.addHeader("Content-Type: " + HttpProcessor::convertContentTypeToString(ContentType::TEXT_HTML) + "\n");
	response.addHeader("\n");
	response.setPayload(output.toLocal8Bit());
	response.setIsStream(false);

	return response;
}

QString EndpointController::getEndpointHelpTemplate(QList<Endpoint>* endpoint_list)
{
	QString output;
	QTextStream stream(&output);

	stream << HtmlEngine::getPageHeader();
	stream << HtmlEngine::getApiHelpHeader("API Help Page");

	for (int i = 0; i < endpoint_list->count(); ++i)
	{
		QList<QString> param_names {};
		QList<QString> param_desc {};

		QMapIterator<QString, ParamProps> p((*endpoint_list)[i].params);
		while (p.hasNext()) {
			p.next();
			param_names.append(p.key());
			param_desc.append(p.value().comment);
		}

		HttpRequest request;
		request.setMethod((*endpoint_list)[i].method);

		stream << HtmlEngine::getApiHelpEntry(
					  (*endpoint_list)[i].url,
					  request.methodAsString(),
					  param_names,
					  param_desc,
					  (*endpoint_list)[i].comment
					);
	}

	stream << HtmlEngine::getPageFooter();

	return output;
}

QString EndpointController::generateGlobalHelp()
{
	return getEndpointHelpTemplate(EndpointManager::getEndpointEntities());
}

QString EndpointController::generateEntityHelp(QString path, RequestMethod method)
{
	QList<Endpoint> selected_endpoints;
	selected_endpoints.append(EndpointManager::getEndpointEntity(path, method));
	return getEndpointHelpTemplate(&selected_endpoints);
}

QString EndpointController::getServedFileLocation(QString file_id)
{
	QString server_root = ServerHelper::getStringSettingsValue("server_root");
	if (!server_root.endsWith(QDir::separator()))
	{
		server_root = server_root + QDir::separator();
	}

	QString served_file = server_root.trimmed() + file_id;
	if (QFile(served_file).exists())
	{
		return served_file;
	}

	UrlEntity url_entity = UrlManager::getURLById(file_id);
	if (!url_entity.filename_with_path.isEmpty())
	{
		return url_entity.filename_with_path;
	}

	return "";
}

bool EndpointController::isEligibileToAccess(HttpRequest request)
{
	if ((!request.getFormUrlEncoded().contains("token")) && (!request.getUrlParams().contains("token")))
	{
		return false;
	}
	if ((!SessionManager::isTokenValid(request.getFormUrlEncoded()["token"])) && (!SessionManager::isTokenValid(request.getUrlParams()["token"])))
	{
		return false;
	}
	return true;
}

QString EndpointController::getFileNameWithExtension(QString filename_with_path)
{
	QList<QString> path_items = filename_with_path.split(QDir::separator());
	return path_items.takeLast();
}

StaticFile EndpointController::readFileContent(QString filename, ByteRange byte_range)
{
	qDebug() << "Reading file:" + filename;
	StaticFile static_file {};
	static_file.filename_with_path = filename;
	static_file.modified = QFileInfo(filename).lastModified();

	QString found_id = FileCache::getFileIdIfInCache(filename);
	if (found_id.length() > 0)
	{
		qDebug() << "File has been found in the cache:" + found_id;
		return FileCache::getFileById(found_id);
	}


	QFile file(filename);
	static_file.size = file.size();
	if (!file.open(QIODevice::ReadOnly))
	{
		THROW(FileAccessException, "File could not be found: " + filename);
	}

	if ((!file.atEnd()) && (byte_range.length == 0))
	{
		qDebug() << "Reading the entire file at once";
		static_file.content = file.readAll();
	}

	if ((!file.atEnd()) && (byte_range.length > 0) && (file.seek(byte_range.start)))
	{
		qDebug() << "Partial file reading";
		static_file.content = file.read(byte_range.length);
	}

//	if (!content.isEmpty())
//	{
//		qDebug() << "Adding file to the cache:" << filename;
//		FileCache::addFileToCache(ServerHelper::generateUniqueStr(), filename, content);
//	}

	file.close();

	return static_file;
}

QString EndpointController::addFileToCache(QString filename)
{
	readFileContent(filename, ByteRange{});
	return FileCache::getFileIdIfInCache(filename);
}


