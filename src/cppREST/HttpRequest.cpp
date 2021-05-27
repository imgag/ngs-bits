#include "HttpRequest.h"

HttpRequest::HttpRequest()
{
}

void HttpRequest::setMethod(RequestMethod type)
{
	method_ = type;
}

RequestMethod HttpRequest::getMethod() const
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

ContentType HttpRequest::getContentType() const
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

QMap<QString, QString> HttpRequest::getHeaders() const
{
	return headers_;
}

QString HttpRequest::getHeaderByName(QString key) const
{
	if (headers_.contains(key.toLower()))
	{
		return headers_.value(key.toLower());
	}

	return "";
}

void HttpRequest::setBody(QByteArray body)
{
	body_ = body;
}

QByteArray HttpRequest::getBody() const
{
	return body_;
}

void HttpRequest::setPrefix(QString prefix)
{
	prefix_ = prefix;
}

QString HttpRequest::getPrefix() const
{
	return prefix_;
}

void HttpRequest::setPath(QString path)
{
	path_ = path;
}

QString HttpRequest::getPath() const
{
	return path_;
}

void HttpRequest::setRemoteAddress(QString address)
{
	remote_address_ = address;
}

QString HttpRequest::getRemoteAddress() const
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

QMap<QString, QString> HttpRequest::getUrlParams() const
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

QMap<QString, QString> HttpRequest::getFormUrlEncoded() const
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

QList<QString> HttpRequest::getPathParams() const
{
	return path_params_;
}
