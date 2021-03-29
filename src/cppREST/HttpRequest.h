#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "cppREST_global.h"
#include "HttpParts.h"
#include "HttpProcessor.h"

class CPPRESTSHARED_EXPORT HttpRequest
{
public:
	HttpRequest();

	void setMethod(RequestMethod type);
	RequestMethod getMethod();
	QString methodAsString();

	void setContentType(ContentType type);
	ContentType getContentType();

	void addHeader(QString key, QString value);
	QMap<QString, QString> getHeaders();

	void setPrefix(QString prefix);
	QString getPrefix();

	void setPath(QString path);
	QString getPath();

	void setRemoteAddress(QString address);
	QString getRemoteAddress();

	void addUrlParam(QString key, QString value);
	void setUrlParams(QMap<QString, QString> params);
	QMap<QString, QString> getUrlParams();

	void addFormUrlEncoded(QString key, QString value);
	void setFormUrlEncoded(QMap<QString, QString> form_params);
	QMap<QString, QString> getFormUrlEncoded();

	void addPathParam(QString param);
	void setPathParams(QList<QString> params);
	QList<QString> getPathParams();

private:
	RequestMethod method_;
	ContentType return_type_;
	QMap<QString, QString> headers_;
	QString prefix_;
	QString path_;
	QString remote_address_;
	QMap<QString, QString> url_params_;
	QMap<QString, QString> form_urlencoded_;
	QList<QString> path_params_;
};

#endif // HTTPREQUEST_H
