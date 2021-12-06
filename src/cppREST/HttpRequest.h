#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "cppREST_global.h"
#include "HttpProcessor.h"

class CPPRESTSHARED_EXPORT HttpRequest
{
public:
	HttpRequest();

	void setMethod(RequestMethod type);
	RequestMethod getMethod() const;
	QString methodAsString();

	void setContentType(ContentType type);
	ContentType getContentType() const;

	void addHeader(QString key, QString value);
	QMap<QString, QList<QString>> getHeaders() const;
	QList<QString> getHeaderByName(QString key) const;

	void setBody(QByteArray body);
	QByteArray getBody() const;

	void setPrefix(QString prefix);
	QString getPrefix() const;

	void setPath(QString path);
	QString getPath() const;

	void setRemoteAddress(QString address);
	QString getRemoteAddress() const;

	void addUrlParam(QString key, QString value);
	void setUrlParams(QMap<QString, QString> params);
	QMap<QString, QString> getUrlParams() const;

	void addFormUrlEncoded(QString key, QString value);
	void setFormUrlEncoded(QMap<QString, QString> form_params);
	QMap<QString, QString> getFormUrlEncoded() const;

	void addPathParam(QString param);
	void setPathParams(QList<QString> params);
	QList<QString> getPathParams() const;

private:
	RequestMethod method_;
	ContentType return_type_;
	QMap<QString, QList<QString>> headers_;
	QByteArray body_;
	QString prefix_;
	QString path_;
	QString remote_address_;
	QMap<QString, QString> url_params_;
	QMap<QString, QString> form_urlencoded_;
	QList<QString> path_params_;
};

#endif // HTTPREQUEST_H
