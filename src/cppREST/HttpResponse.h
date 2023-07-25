#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "cppREST_global.h"
#include <QByteArray>
#include <QString>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include "HttpUtils.h"
#include "HtmlEngine.h"
#include "ServerHelper.h"

class CPPRESTSHARED_EXPORT HttpResponse : public QByteArray
{
public:
	HttpResponse();
	HttpResponse(BasicResponseData data);
	HttpResponse(BasicResponseData data, QByteArray payload);
	HttpResponse(ResponseStatus status, ContentType content_type, qlonglong content_length);
	HttpResponse(ResponseStatus status, ContentType content_type, QString message);  

	void setIsStream(bool is_stream);
	bool isStream();

	void setFilename(QString filename);
	QString getFilename() const;

	void setStatus(ResponseStatus response_status);
	ResponseStatus getStatus() const;

	void setByteRanges(QList<ByteRange> ranges);
	QList<ByteRange> getByteRanges() const;

	QByteArray getStatusLine() const;

	int getStatusCode() const;

	void setHeaders(QByteArray headers);
	void addHeader(QByteArray header);
	QByteArray getHeaders() const;

	void setBoundary(QByteArray boundary);
	QByteArray getBoundary() const;

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
	QByteArray multipart_boundary_;
	QByteArray payload_;
	QList<ByteRange> ranges_;
};

#endif // HTTPRESPONSE_H
