#include "HttpRequest.h"

HttpRequest::HttpRequest()
{
}

void HttpRequest::setMethod(RequestMethod type)
{
	method_ = type;
}

RequestMethod HttpRequest::getMethod()
{
	return method_;
}

QString HttpRequest::methodAsString()
{
	return HttpProcessor::convertMethodTypeToString(method_);
}


void HttpRequest::setContentType(ContentType type)
{
	return_type_ = type;
}

ContentType HttpRequest::getContentType()
{
	return return_type_;
}

void HttpRequest::addHeader(QString key, QString value)
{
	if (!headers_.contains(key))
	{
		headers_.insert(key, value);
	}
}

QMap<QString, QString> HttpRequest::getHeaders()
{
	return headers_;
}

void HttpRequest::setBody(QByteArray body)
{
	body_ = body;
}

QByteArray HttpRequest::getBody()
{
	return body_;
}

void HttpRequest::setPrefix(QString prefix)
{
	prefix_ = prefix;
}

QString HttpRequest::getPrefix()
{
	return prefix_;
}

void HttpRequest::setPath(QString path)
{
	path_ = path;
}

QString HttpRequest::getPath()
{
	return path_;
}

void HttpRequest::setRemoteAddress(QString address)
{
	remote_address_ = address;
}

QString HttpRequest::getRemoteAddress()
{
	return remote_address_;
}

void HttpRequest::addUrlParam(QString key, QString value)
{
	if (!url_params_.contains(key))
	{
		url_params_.insert(key, value);
	}
}

void HttpRequest::setUrlParams(QMap<QString, QString> params)
{
	url_params_ = params;
}

QMap<QString, QString> HttpRequest::getUrlParams()
{
	return url_params_;
}

void HttpRequest::addFormUrlEncoded(QString key, QString value)
{
	if (!form_urlencoded_.contains(key))
	{
		form_urlencoded_.insert(key, value);
	}
}

void HttpRequest::setFormUrlEncoded(QMap<QString, QString> form_params)
{
	form_urlencoded_ = form_params;
}

QMap<QString, QString> HttpRequest::getFormUrlEncoded()
{
	return form_urlencoded_;
}

void HttpRequest::addPathParam(QString param)
{
	path_params_.append(param);
}

void HttpRequest::setPathParams(QList<QString> params)
{
	path_params_ = params;
}

QList<QString> HttpRequest::getPathParams()
{
	return path_params_;
}
