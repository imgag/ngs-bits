#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "cppREST_global.h"
#include "HttpUtils.h"

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
	const QMap<QString, QList<QString>>& getHeaders() const;
	QList<QString> getHeaderByName(QString key) const;

	void setBody(QByteArray body);
	QByteArray getBody() const;

	void addFormDataParam(QString key, QString value);
	const QMap<QString, QString>& getFormDataParams() const;

	void setMultipartFileName(QString file);
	QString getMultipartFileName() const;

	void setMultipartFileContent(QByteArray content);
	QByteArray getMultipartFileContent() const;

	void setPrefix(QString prefix);
	QString getPrefix() const;

	void setPath(QString path);
	QString getPath() const;

	void setRemoteAddress(QString address);
	QString getRemoteAddress() const;

	void addUrlParam(QString key, QString value);
	void setUrlParams(const QMap<QString, QString>& params);
	const QMap<QString, QString>& getUrlParams() const;

	void addFormUrlEncoded(QString key, QString value);
	void setFormUrlEncoded(const QMap<QString, QString>& form_params);
	const QMap<QString, QString>& getFormUrlEncoded() const;

	void addPathItem(QString param);
	void setPathItems(const QList<QString>& params);
	QList<QString> getPathItems() const;

private:
	RequestMethod method_;
	ContentType return_type_;
	QMap<QString, QList<QString>> headers_;
	QByteArray body_;
	QMap<QString, QString> form_data_params_;
	QString multipart_file_name_;
	QByteArray multipart_file_content_;
	QString prefix_;
	QString path_;
	QString remote_address_;
	QMap<QString, QString> url_params_;
	QMap<QString, QString> form_urlencoded_;
	QList<QString> path_items_;
};

#endif // HTTPREQUEST_H
