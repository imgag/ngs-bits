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
	HttpResponse(BasicResponseData data);
	HttpResponse(BasicResponseData data, QByteArray payload);
//	HttpResponse(bool is_stream, bool is_binary, QString filename, QByteArray headers, QByteArray payload);
	HttpResponse(ResponseStatus status, ContentType content_type, QString message);

	void setIsStream(bool is_stream);
	bool isStream();

	void setIsBinary(bool is_binary);
	bool isBinary();

	void setFilename(QString filename);
	QString getFilename();

	void setStatusLine(ResponseStatus response_status);
	QByteArray getStatusLine();

	void setHeaders(QByteArray headers);
	void addHeader(QString header);
	QByteArray getHeaders();

	void setPayload(QByteArray payload);
	QByteArray getPayload();

	void setRangeNotSatisfiableHeaders(BasicResponseData data);

private:
	void readBasicResponseData(BasicResponseData data);
	QByteArray generateRegularHeaders(BasicResponseData data);
	QByteArray generateChunkedStreamHeaders(BasicResponseData data);
	QByteArray generateRangeNotSatisfiableHeaders(BasicResponseData data);

protected:
	bool is_stream_;
	bool is_binary_;
	QString filename_;
	QByteArray status_line_;
	QByteArray headers_;
	QByteArray payload_;
	int getContentLength();
	void updateResponseData();
};

#endif // HTTPRESPONSE_H
