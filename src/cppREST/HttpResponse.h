#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "cppREST_global.h"
#include <QByteArray>
#include <QString>
#include <QDir>
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
	HttpResponse(ResponseStatus status, ContentType content_type, QString message);

	void setIsStream(bool is_stream);
	bool isStream();

	void setFilename(QString filename);
	QString getFilename() const;

	void setStatus(ResponseStatus response_status);
	ResponseStatus getStatus() const;

	QByteArray getStatusLine() const;

	int getStatusCode() const;

	void setHeaders(QByteArray headers);
	void addHeader(QString header);
	QByteArray getHeaders() const;

	void setPayload(QByteArray payload);
	QByteArray getPayload() const;

	void setRangeNotSatisfiableHeaders(BasicResponseData data);

private:
	void readBasicResponseData(BasicResponseData data);
	QByteArray generateRegularHeaders(BasicResponseData data);
	QByteArray generateChunkedStreamHeaders(BasicResponseData data);
	QByteArray generateRangeNotSatisfiableHeaders(BasicResponseData data);
	QString getFileNameWithExtension(QString filename_with_path) const;
	int getContentLength() const;
	void updateResponseData();

protected:
	bool is_stream_;
	QString filename_;
	ResponseStatus response_status_;
	QByteArray status_line_;
	QByteArray headers_;
	QByteArray payload_;
};

#endif // HTTPRESPONSE_H
