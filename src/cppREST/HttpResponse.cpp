#include "HttpResponse.h"

HttpResponse::HttpResponse()
{
	this->clear();
}

HttpResponse::HttpResponse(QByteArray response_data)
{
	this->clear();
	this->append(response_data);
}

HttpResponse::HttpResponse(BasicResponseData data)
{
	readBasicResponseData(data);
}

HttpResponse::HttpResponse(BasicResponseData data, QByteArray payload)
	: payload_(payload)
{
	readBasicResponseData(data);
}

//HttpResponse::HttpResponse(bool is_stream, bool is_binary, QString filename, QByteArray headers, QByteArray payload)
//	: is_stream_(is_stream)
//	, is_binary_(is_binary)
//	, filename_(filename)
//	, headers_(headers)
//	, payload_(payload)
//{
//}

HttpResponse::HttpResponse(ResponseStatus status, ContentType content_type, QString message)
{
	if (message.isEmpty())
	{
		message	= "Unknown error has been detected";
	}

	switch (content_type)
	{
		case ContentType::TEXT_HTML:
		{
			QString html_body = HtmlEngine::getErrorPageTemplate();
			html_body.replace("%TITLE%", "Error " + QString::number(HttpProcessor::convertResponseStatusToStatusCode(status)) + " - " + HttpProcessor::convertResponseStatusToReasonPhrase(status));
			html_body.replace("%MESSAGE%", message);
			setPayload(html_body.toLocal8Bit());
		}
		break;
		case ContentType::APPLICATION_JSON:
		{
			QJsonDocument json_doc_output {};
			QJsonObject json_object {};
			json_object.insert("message", message);
			json_object.insert("code", HttpProcessor::convertResponseStatusToStatusCode(status));
			json_object.insert("type", HttpProcessor::convertResponseStatusToReasonPhrase(status));

			json_doc_output.setObject(json_object);
			setPayload(json_doc_output.toJson());
		}
		break;
		default:
		{
			content_type = ContentType::TEXT_PLAIN;
			setPayload(message.toLocal8Bit());
		}
	}

	setStatusLine(status);
	addHeader("Content-Length: " + QString::number(getContentLength()) + "\r\n");
	addHeader("Content-Type: " + HttpProcessor::convertContentTypeToString(content_type) + "\r\n");
	addHeader(QString("\r\n"));
	setIsStream(false);
}

void HttpResponse::setIsStream(bool is_stream)
{
	is_stream_ = is_stream;
}

bool HttpResponse::isStream()
{
	return is_stream_;
}

void HttpResponse::setIsBinary(bool is_binary)
{
	is_binary_ = is_binary;
}

bool HttpResponse::isBinary()
{
	return is_binary_;
}

void HttpResponse::setFilename(QString filename)
{
	filename_ = filename;
}

QString HttpResponse::getFilename()
{
	return filename_;
}

void HttpResponse::setStatusLine(ResponseStatus response_status)
{
	status_line_ = "HTTP/1.1 " + QByteArray::number(HttpProcessor::convertResponseStatusToStatusCode(response_status))
			+ " " + HttpProcessor::convertResponseStatusToReasonPhrase(response_status).toLocal8Bit() + "\r\n";
}

QByteArray HttpResponse::getStatusLine()
{
	return status_line_;
}

void HttpResponse::setHeaders(QByteArray headers)
{
	headers_ = headers;
	updateResponseData();
}

void HttpResponse::addHeader(QString header)
{
	headers_.append(header);
	updateResponseData();
}

QByteArray HttpResponse::getHeaders()
{
	return headers_;
}

void HttpResponse::setPayload(QByteArray payload)
{
	payload_ = payload;
	updateResponseData();
}

QByteArray HttpResponse::getPayload()
{
	return payload_;
}

void HttpResponse::setRangeNotSatisfiableHeaders(BasicResponseData data)
{
	setHeaders(generateRangeNotSatisfiableHeaders(data));
}

void HttpResponse::readBasicResponseData(BasicResponseData data)
{
	qDebug() << "Creating a response object";
	qDebug() << "data.filename 1" << data.filename;
	if ((data.byte_range.end > 0) && (data.byte_range.length > 0))
	{
		setStatusLine(ResponseStatus::PARTIAL_CONTENT);
	}
	else
	{
		setStatusLine(ResponseStatus::OK);
	}

	setIsStream(data.is_stream);
	if (data.is_stream)
	{
		ContentType content_type = HttpProcessor::getContentTypeByFilename(data.filename);
		setIsBinary(false);
		if ((content_type == APPLICATION_OCTET_STREAM) || (content_type == IMAGE_PNG) || (content_type == IMAGE_JPEG))
		{
			setIsBinary(true);
		}
		qDebug() << "data.filename" << data.filename;
		setFilename(data.filename);
	}

	setHeaders(generateRegularHeaders(data));
}

QByteArray HttpResponse::generateRegularHeaders(BasicResponseData data)
{
	QByteArray headers;
	headers.append("Date: " + QDateTime::currentDateTime().toUTC().toString() + "\r\n");
	headers.append("Content-Length: " + QString::number(data.length) + "\r\n");
	headers.append("Content-Type: " + HttpProcessor::convertContentTypeToString(data.content_type) + "\r\n");
	headers.append("Connection: Keep-Alive\r\n");
	if ((data.byte_range.end > 0) && (data.byte_range.length > 0))
	{
		headers.append("Accept-Ranges: bytes\r\n");
		headers.append("Content-Range: bytes " + QString::number(data.byte_range.start) + "-" + QString::number(data.byte_range.end) + "/" + QString::number(data.file_size) + "\r\n");
		qDebug() << "Content-Range: bytes " << QString::number(data.byte_range.start) << "-" << QString::number(data.byte_range.end) << "/" << QString::number(data.file_size);
	}
	if (data.is_downloadable)
	{
		headers.append("Content-Disposition: form-data; name=file_download; filename=" + data.filename + "\r\n");
	}

	headers.append("\r\n");
	return headers;
}

QByteArray HttpResponse::generateChunkedStreamHeaders(BasicResponseData data)
{
	QByteArray headers;
	headers.append("Date: " + QDateTime::currentDateTime().toUTC().toString() + "\r\n");
	headers.append("Content-Type: " + HttpProcessor::convertContentTypeToString(data.content_type) + "\r\n");
	headers.append("Connection: Keep-Alive\r\n");
	headers.append("Transfer-Encoding: chunked\r\n");

	if (data.is_downloadable)
	{
		headers.append("Content-Disposition: form-data; name=file_download; filename=" + data.filename + "\r\n");
	}

	headers.append("\r\n");
	return headers;
}

QByteArray HttpResponse::generateRangeNotSatisfiableHeaders(BasicResponseData data)
{
	QByteArray headers;
	headers.append("Date: " + QDateTime::currentDateTime().toUTC().toString() + "\r\n");
	headers.append("Content-Range: bytes */" + QString::number(data.file_size) + "\r\n");
	headers.append("\r\n");
	return headers;
}

int HttpResponse::getContentLength()
{
	return payload_.length();
}

void HttpResponse::updateResponseData()
{
	this->clear();
	this->append(headers_);
	this->append(payload_);
}
