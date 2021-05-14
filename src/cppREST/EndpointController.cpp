#include "EndpointController.h"

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
	return HttpResponse(response_data, body);
}

HttpResponse EndpointController::serveStaticFromServerRoot(HttpRequest request)
{
	QString served_file = getServedRootPath(request.getPathParams());

	if (served_file.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), "Location has not been found in the server root folder");
	}

	if (QFileInfo(served_file).isDir())
	{
		return serveFolderContent(served_file, request.getPrefix(), request.getPath(), request.getPathParams());
	}

	return serveStaticFile(served_file, request.getMethod(), request.getHeaders());
}

HttpResponse EndpointController::serveStaticForTempUrl(HttpRequest request)
{
	QString served_file;
	try
	{
		served_file = getServedTempPath(request.getPathParams());
	}
	catch(Exception& e)
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), e.message());
	}

	if (QFileInfo(served_file).isDir())
	{
		return serveFolderContent(served_file, request.getPrefix(), request.getPath(), request.getPathParams());
	}

	return serveStaticFile(served_file, request.getMethod(), request.getHeaders());
}

HttpResponse EndpointController::serveStaticFileFromCache(HttpRequest request)
{
	QString filename = FileCache::getFileById(request.getPathParams().value(0)).filename_with_path;
	return createStaticFromCacheResponse(filename, ByteRange{}, HttpProcessor::getContentTypeByFilename(filename), false);
}

HttpResponse EndpointController::serveProtectedStaticFile(HttpRequest request)
{
	if (!isEligibileToAccess(request))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "Secure token has not been provided");
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
			UrlEntity current_entity = UrlManager::getURLById(url_parts[url_parts.size()-2]);
			if (!current_entity.path.isEmpty())
			{
				filename = current_entity.path + QDir::separator() + url_parts.value(url_parts.size()-1);
			}
		}
	}

	if (!QFile(filename).exists())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, request.getContentType(), "File does not exist: " + filename);
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

	return HttpResponse(response_data, json_doc_output.toJson());
}

HttpResponse EndpointController::createStaticFileResponse(QString filename, ByteRange byte_range, ContentType type, bool is_downloadable)
{
	StaticFile static_file;

	try
	{
		static_file = readFileContent(filename, byte_range);
	}
	catch(Exception& e)
	{
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, e.message());
	}

	BasicResponseData response_data;
	response_data.filename = filename;
	response_data.length = static_file.content.length();
	response_data.byte_range = byte_range;
	response_data.file_size = QFile(filename).size();
	response_data.content_type = type;
	response_data.is_downloadable = is_downloadable;

	qDebug() << "response_data.file_size = " << response_data.file_size;
	return HttpResponse(response_data, static_file.content);
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

HttpResponse EndpointController::createStaticFromCacheResponse(QString id, ByteRange byte_range, ContentType type, bool is_downloadable)
{
	StaticFile static_file = FileCache::getFileById(id);

	if (static_file.content.isEmpty() || static_file.content.isNull())
	{
		return HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Empty or corrpupted file");
	}

	BasicResponseData response_data;
	response_data.filename = FileCache::getFileById(id).filename_with_path;
	response_data.length = static_file.content.length();
	response_data.byte_range = byte_range;
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
	QDir dir(path);
	if (!dir.exists())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, "Requested folder does not exist");
	}

	QString base_folder_url = ServerHelper::getStringSettingsValue("server_host") + ":" + QString::number(ServerHelper::getNumSettingsValue("server_port")) + "/" + request_prefix + "/" + request_path;
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

HttpResponse EndpointController::serveStaticFile(QString filename, RequestMethod method, QMap<QString, QString> headers)
{
	if (filename.isEmpty())
	{
		return HttpResponse(ResponseStatus::NOT_FOUND, ContentType::TEXT_HTML, "Requested file does not exist");
	}

	ByteRange byte_range {};
	byte_range.start = 0;
	byte_range.end = 0;
	if (headers.contains("range"))
	{
		QString range_value = headers.value("range");
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

	if (method == RequestMethod::HEAD)
	{
		qDebug() << "Processing HEAD method";
		BasicResponseData response_data;
		response_data.length = QFileInfo(filename).size();
		response_data.filename = filename;
		response_data.file_size = QFileInfo(filename).size();
		response_data.content_type = HttpProcessor::getContentTypeByFilename(filename);
		response_data.is_stream = false;

		return HttpResponse(response_data);
	}

	if (!headers.contains("range"))
	{
		qDebug() << "Processing STREAM";
		return createStaticStreamResponse(filename, false);
	}
	qDebug() << "Processing RANGE";
	return createStaticFileResponse(filename, byte_range, HttpProcessor::getContentTypeByFilename(filename), false);
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

QString EndpointController::getEndpointHelpTemplate(QList<Endpoint>* endpoint_list)
{
	QString output;
	QTextStream stream(&output);

	stream << HtmlEngine::getPageHeader("API Help Page");
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


QString EndpointController::getServedTempPath(QList<QString> path_parts)
{
	if (path_parts.size() < 1)
	{
		THROW(Exception, "Not path has been provided in temporary URL");
	}

	UrlEntity url_entity = UrlManager::getURLById(path_parts.value(0));
	if (!url_entity.filename_with_path.isEmpty())
	{
		path_parts.removeAt(0);
		return QFileInfo(url_entity.filename_with_path).absolutePath() + QDir::separator() + path_parts.join(QDir::separator());
	}

	return "";
}

QString EndpointController::getServedRootPath(QList<QString> path_parts)
{
	QString server_root = ServerHelper::getStringSettingsValue("server_root");
	if (!server_root.endsWith(QDir::separator()))
	{
		server_root = server_root + QDir::separator();
	}

	QString served_file = server_root.trimmed() + path_parts.join(QDir::separator());
	served_file = QUrl::fromEncoded(served_file.toLocal8Bit()).toString(); // handling browser endcoding, e.g. spaces and other characters in names

	if (QFile(served_file).exists())
	{
		return served_file;
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
		try
		{
			static_file.content = file.readAll();
		}
		catch (FileAccessException& e)
		{
			THROW(FileAccessException, "File cannot be processed (possibly due to the large size)");
		}
	}

	if ((!file.atEnd()) && (byte_range.length > 0) && (file.seek(byte_range.start)))
	{
		qDebug() << "Partial file reading";
		static_file.content = file.read(byte_range.length);
	}

	if ((!static_file.content.isEmpty()) && (Settings::boolean("static_cache", true)))
	{
		qDebug() << "Adding file to the cache:" << filename;
		FileCache::addFileToCache(ServerHelper::generateUniqueStr(), filename, static_file.content, static_file.content.size());
	}

	file.close();

	return static_file;
}

QString EndpointController::addFileToCache(QString filename)
{
	readFileContent(filename, ByteRange{});
	return FileCache::getFileIdIfInCache(filename);
}


