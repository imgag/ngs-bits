#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "cppREST_global.h"
#include <QByteArray>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include "HttpParts.h"
#include "HttpProcessor.h"
#include "HtmlEngine.h"

class CPPRESTSHARED_EXPORT HttpResponse : public QByteArray
{
public:
	HttpResponse();
	HttpResponse(QByteArray response_data);
	HttpResponse(bool is_stream, QString filename, QByteArray headers, QByteArray payload);
	HttpResponse(HttpError error);

	void setIsStream(bool is_stream);
	bool isStream();

	void setFilename(QString filename);
	QString getFilename();

	void setHeaders(QByteArray headers);
	void addHeader(QString header);
	QByteArray getHeaders();

	void setPayload(QByteArray payload);
	QByteArray getPayload();	

protected:
	bool is_stream_;
	QString filename_;
	QByteArray headers_;
	QByteArray payload_;
	int getContentLength();
	void updateResponseData();
};

#endif // HTTPRESPONSE_H
