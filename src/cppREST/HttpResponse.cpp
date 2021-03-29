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

HttpResponse::HttpResponse(bool is_stream, QString filename, QByteArray headers, QByteArray payload)
	: is_stream_(is_stream)
	, filename_(filename)
	, headers_(headers)
	, payload_(payload)
{
}

HttpResponse::HttpResponse(HttpError error)
{
	ContentType content_type = error.content_type;
	QString message = error.error_message;
	if (message.isEmpty())
	{
		message	= "Unknown error has been detected";
	}

	switch (content_type)
	{
		case ContentType::TEXT_HTML:
		{
			QString html_body = HtmlEngine::getErrorPageTemplate();
			html_body.replace("%TITLE%", "Error " + QString::number(HttpProcessor::convertStatusCodeToInt(error.status_code)) + " - " + HttpProcessor::convertStatusCodeToReasonPhrase(error.status_code));
			html_body.replace("%MESSAGE%", message);
			setPayload(html_body.toLocal8Bit());
		}
		break;
		case ContentType::APPLICATION_JSON:
		{
			QJsonDocument json_doc_output {};
			QJsonObject json_object {};
			json_object.insert("message", message);
			json_object.insert("code", HttpProcessor::convertStatusCodeToInt(error.status_code));
			json_object.insert("type", HttpProcessor::convertStatusCodeToReasonPhrase(error.status_code));

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

	addHeader("HTTP/1.1 " + QString::number(HttpProcessor::convertStatusCodeToInt(error.status_code)) + " FAIL\r\n");
	addHeader("Content-Length: " + QString::number(getContentLength()) + "\r\n");
	addHeader("Content-Type: " + HttpProcessor::convertContentTypeToString(content_type) + "\r\n");
	addHeader(QString("\r\n"));
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

QString HttpResponse::getFilename()
{
	return filename_;
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
