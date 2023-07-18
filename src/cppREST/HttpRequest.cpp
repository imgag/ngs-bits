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
	return HttpUtils::convertMethodTypeToString(method_);
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
	if (value.trimmed().length() == 0) return;
	if (!headers_.contains(key.toLower()))
	{
		headers_[key] = QList<QString>{};
	}
	if (!headers_[key].contains(value.toLower()))
	{
		headers_[key].append(value.trimmed());
	}
}

const QMap<QString, QList<QString>>& HttpRequest::getHeaders() const
{
	return headers_;
}

QList<QString> HttpRequest::getHeaderByName(QString key) const
{
	if (headers_.contains(key.toLower()))
	{
		return headers_[key.toLower()];
	}

	return {};
}

void HttpRequest::setBody(QByteArray body)
{
	body_ = body;
}

QByteArray HttpRequest::getBody() const
{
	return body_;
}

void HttpRequest::addFormDataParam(QString key, QString value)
{
	form_data_params_.insert(key, value);
}

const QMap<QString, QString>& HttpRequest::getFormDataParams() const
{
	return form_data_params_;
}

void HttpRequest::setMultipartFileName(QString file)
{
	multipart_file_name_ = file;
}

QString HttpRequest::getMultipartFileName() const
{
	return multipart_file_name_;
}

void HttpRequest::setMultipartFileContent(QByteArray content)
{
	multipart_file_content_ = content;
}

QByteArray HttpRequest::getMultipartFileContent() const
{
	return multipart_file_content_;
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

void HttpRequest::setUrlParams(const QMap<QString, QString>& params)
{
	url_params_ = params;
}

const QMap<QString, QString>& HttpRequest::getUrlParams() const
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

void HttpRequest::setFormUrlEncoded(const QMap<QString, QString>& form_params)
{
	form_urlencoded_ = form_params;
}

const QMap<QString, QString>& HttpRequest::getFormUrlEncoded() const
{
	return form_urlencoded_;
}

void HttpRequest::addPathItem(QString param)
{
	path_items_.append(param);
}

void HttpRequest::setPathItems(const QList<QString>& params)
{
	path_items_ = params;
}

QList<QString> HttpRequest::getPathItems() const
{
	return path_items_;
}
