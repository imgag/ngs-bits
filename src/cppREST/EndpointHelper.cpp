#include "EndpointHelper.h"

bool EndpointHelper::isEligibileToAccess(HttpRequest request)
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

QString EndpointHelper::getFileNameWithExtension(QString filename_with_path)
{
	QList<QString> path_items = filename_with_path.split('/');
	return path_items.takeLast();
}

StaticFile EndpointHelper::readFileContent(QString filename, ByteRange byte_range)
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

QString EndpointHelper::addFileToCache(QString filename)
{
	readFileContent(filename, ByteRange{});
	return FileCache::getFileIdIfInCache(filename);
}

HttpResponse EndpointHelper::serveStaticFile(QString filename, ByteRange byte_range, ContentType type, bool is_downloadable)
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

	return HttpResponse(false, false, "", generateHeaders(response_data), static_file.content);
}

HttpResponse EndpointHelper::serveStaticFileFromCache(QString id, ByteRange byte_range, ContentType type, bool is_downloadable)
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

	return HttpResponse(false, false, "", generateHeaders(response_data), static_file.content);
}

HttpResponse EndpointHelper::streamStaticFile(QString filename, bool is_downloadable)
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

HttpResponse EndpointHelper::serveFolderContent(QString folder)
{
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
		qDebug() << "File:" + fileInfo.fileName() + ", " + fileInfo.size() + fileInfo.isDir();
	}
	return serveFolderListing(files);
}

QByteArray EndpointHelper::generateHeaders(BasicResponseData data)
{
	QByteArray headers;
	if ((data.byte_range.end > 0) && (data.byte_range.length > 0))
	{
		headers.append("HTTP/1.1 206 Partial Content\r\n");
	}
	else
	{
		headers.append("HTTP/1.1 200 OK\r\n");
	}
	headers.append("Date: " + QDateTime::currentDateTime().toUTC().toString() + "\r\n");
	headers.append("Server: " + ServerHelper::getAppName() + "\r\n");
	headers.append("Connection: Keep-Alive\r\n");
	headers.append("Keep-Alive: timeout=5, max=1000\r\n");
	headers.append("Content-Length: " + QString::number(data.length) + "\r\n");
	headers.append("Content-Type: " + HttpProcessor::convertContentTypeToString(data.content_type) + "\r\n");

	if ((data.byte_range.end > 0) && (data.byte_range.length > 0))
	{
		headers.append("Accept-Ranges: bytes\r\n");
		headers.append("Content-Range: bytes " + QString::number(data.byte_range.start) + "-" + QString::number(data.byte_range.end) + "/" + QString::number(data.file_size) + "\r\n");
	}
	if (data.is_downloadable)
	{
		headers.append("Content-Disposition: form-data; name=file_download; filename=" + data.filename + "\r\n");
	}

	headers.append("\r\n");
	return headers;
}

HttpResponse EndpointHelper::listFolderContent(HttpRequest request)
{
	return serveFolderContent("./");
}

HttpResponse EndpointHelper::serveEndpointHelp(HttpRequest request)
{
	QByteArray body {};
	if (request.getPathParams().count() == 0)
	{
		body = EndpointManager::generateGlobalHelp().toLocal8Bit();
	}
	else
	{
		body = EndpointManager::generateEntityHelp(request.getPathParams()[0], request.getMethod()).toLocal8Bit();
	}
	BasicResponseData response_data;
	response_data.length = body.length();
	response_data.content_type = ContentType::TEXT_HTML;
	response_data.is_downloadable = false;
	return HttpResponse{false, false, "", generateHeaders(response_data), body};
}

HttpResponse EndpointHelper::serveStaticFile(HttpRequest request)
{
	QString server_root = ServerHelper::getStringSettingsValue("server_root");
	if (!server_root.endsWith(QDir::separator()))
	{
		server_root = server_root + QDir::separator();
	}

	QString served_file = server_root.trimmed() + request.getPathParams()[0];
	if (!QFile(served_file).exists())
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
		return streamStaticFile(served_file, false);
	}
	return serveStaticFile(served_file, byte_range, HttpProcessor::getContentTypeByFilename(served_file), false);
}

HttpResponse EndpointHelper::serveStaticFileFromCache(HttpRequest request)
{
	QString path = ServerHelper::getUrlWithoutParams(FileCache::getFileById(request.getPathParams()[0]).filename_with_path);
	return serveStaticFile(path, ByteRange{}, HttpProcessor::getContentTypeByFilename(path), false);
}

HttpResponse EndpointHelper::streamStaticFile(HttpRequest request)
{
	QString path = ServerHelper::getStringSettingsValue("server_root");
	path = ServerHelper::getUrlWithoutParams(path.trimmed() + request.getPathParams()[0]);
	bool downloadable = false;
	if (request.getUrlParams().size() > 0) downloadable = true;
	return streamStaticFile(path, downloadable);
}

HttpResponse EndpointHelper::serveProtectedStaticFile(HttpRequest request)
{
	if (!isEligibileToAccess(request)) return HttpResponse(HttpError{StatusCode::FORBIDDEN, request.getContentType(), "Secure token has not been provided"});

	return serveStaticFile(":/assets/client/example.png", ByteRange{}, ContentType::APPLICATION_OCTET_STREAM, true);
}

HttpResponse EndpointHelper::getFileInfo(HttpRequest request)
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

	return HttpResponse{false, false, "", generateHeaders(response_data), json_doc_output.toJson()};
}

HttpResponse EndpointHelper::getProcessingSystemRegions(HttpRequest request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemRegionsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, ContentType::TEXT_HTML, "Processing system regions file has not been found"});
	}
	return streamStaticFile(filename, false);
}

HttpResponse EndpointHelper::getProcessingSystemAmplicons(HttpRequest request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemAmpliconsFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, ContentType::TEXT_HTML, "Processing system amplicons file has not been found"});
	}
	return streamStaticFile(filename, false);
}

HttpResponse EndpointHelper::getProcessingSystemGenes(HttpRequest request)
{
	NGSD db;
	QString sys_id = request.getUrlParams()["sys_id"];
	QString filename = db.processingSystemGenesFilePath(sys_id.toInt());
	if (filename.isEmpty())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, ContentType::TEXT_HTML, "Processing system genes file has not been found"});
	}
	return streamStaticFile(filename, false);
}

HttpResponse EndpointHelper::serveFolderListing(QList<FolderItem> items)
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
	response.addHeader("Content-Length: " + QString::number(output.length()) + "\n");
	response.addHeader("Content-Type: " + HttpProcessor::convertContentTypeToString(ContentType::TEXT_HTML) + "\n");
	response.addHeader("\n");
	response.setPayload(output.toLocal8Bit());

	return response;
}

EndpointHelper::EndpointHelper()
{
}

EndpointHelper& EndpointHelper::instance()
{
	static EndpointHelper endpoint_helper;
	return endpoint_helper;
}
