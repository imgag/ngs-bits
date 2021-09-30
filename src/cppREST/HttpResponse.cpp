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
			int status_code_number = HttpProcessor::convertResponseStatusToStatusCodeNumber(status);
			QString html_body = HtmlEngine::getResponsePageTemplate(HttpProcessor::convertResponseStatusCodeNumberToStatusClass(status_code_number)
																+ " " + QString::number(status_code_number)
																+ " - " + HttpProcessor::convertResponseStatusToReasonPhrase(status),
																message);
			setPayload(html_body.toLocal8Bit());
		}
		break;
		case ContentType::APPLICATION_JSON:
		{
			QJsonDocument json_doc_output {};
			QJsonObject json_object {};
			json_object.insert("message", message);
			json_object.insert("code", HttpProcessor::convertResponseStatusToStatusCodeNumber(status));
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

	setStatus(status);
	addHeader("Content-Length: " + QString::number(getContentLength()) + "\r\n");
	addHeader("Content-Type: " + HttpProcessor::convertContentTypeToString(content_type) + "\r\n");

	if (HttpProcessor::convertResponseStatusToStatusCodeNumber(status) == 401)
	{
		addHeader("WWW-Authenticate: Basic realm=\"Access to the secure area of GSvar\"\r\n");
	}

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

void HttpResponse::setFilename(QString filename)
{
	filename_ = filename;
}

QString HttpResponse::getFilename() const
{
	return filename_;
}

void HttpResponse::setStatus(ResponseStatus response_status)
{
	response_status_ = response_status;
}

ResponseStatus HttpResponse::getStatus() const
{
	return response_status_;
}

QByteArray HttpResponse::getStatusLine() const
{
	return "HTTP/1.1 " + QByteArray::number(HttpProcessor::convertResponseStatusToStatusCodeNumber(response_status_))
			+ " " + HttpProcessor::convertResponseStatusToReasonPhrase(response_status_).toLocal8Bit() + "\r\n";
}

int HttpResponse::getStatusCode() const
{
	return HttpProcessor::convertResponseStatusToStatusCodeNumber(response_status_);
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

QByteArray HttpResponse::getHeaders() const
{
	return headers_;
}

void HttpResponse::setPayload(QByteArray payload)
{
	payload_ = payload;
	updateResponseData();
}

QByteArray HttpResponse::getPayload() const
{
	return payload_;
}

void HttpResponse::setRangeNotSatisfiableHeaders(BasicResponseData data)
{
	setHeaders(generateRangeNotSatisfiableHeaders(data));
}

void HttpResponse::readBasicResponseData(BasicResponseData data)
{
	setStatus(data.status);
	if ((data.byte_range.end > 0) && (data.byte_range.length > 0))
	{
		setStatus(ResponseStatus::PARTIAL_CONTENT);
	}

	setIsStream(data.is_stream);
	setFilename(data.filename);
	setHeaders(generateRegularHeaders(data));
}

QByteArray HttpResponse::generateRegularHeaders(BasicResponseData data)
{
	QByteArray headers;
	headers.append("Date: " + QDateTime::currentDateTime().toUTC().toString() + "\r\n");
	headers.append("Content-Length: " + QString::number(data.length) + "\r\n");
	headers.append("Content-Type: " + HttpProcessor::convertContentTypeToString(data.content_type) + "\r\n");
	headers.append("Connection: Keep-Alive\r\n");
	if (HttpProcessor::convertResponseStatusToStatusCodeNumber(data.status) == 401)
	{
		headers.append("WWW-Authenticate: Basic realm=\"Access to the secure area of GSvar\"\r\n");
	}
	if ((data.byte_range.end > 0) && (data.byte_range.length > 0))
	{
		headers.append("Accept-Ranges: bytes\r\n");
		headers.append("Content-Range: bytes " + QString::number(data.byte_range.start) + "-" + QString::number(data.byte_range.end) + "/" + QString::number(data.file_size) + "\r\n");
	}
	if (data.is_downloadable)
	{
		headers.append("Content-Disposition: form-data; name=file_download; filename=" + getFileNameWithExtension(data.filename) + "\r\n");
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
		headers.append("Content-Disposition: form-data; name=file_download; filename=" + getFileNameWithExtension(data.filename) + "\r\n");
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

QString HttpResponse::getFileNameWithExtension(QString filename_with_path) const
{
	QList<QString> path_items = filename_with_path.split(QDir::separator());
	return path_items.takeLast();
}

int HttpResponse::getContentLength() const
{
	return payload_.length();
}

void HttpResponse::updateResponseData()
{
	this->clear();
	this->append(headers_);
	this->append(payload_);
}
