#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QObject>
#include <QString>
#include <QNetworkProxy>
#include <QHttpMultiPart>
#include "HttpRequestHandler.h"

using HttpHeaders = QMap<QByteArray, QByteArray>;

///Helper class for HTTP(S) communication with webserver
class HttpHandler
		: public QObject
{
	Q_OBJECT

public:
	///Constructor
	HttpHandler(bool internal, QObject* parent=0);

	///Returns basic headers used for all get/post requests. Additional headers that are only used for one request can be given in the get/post methods.
	const HttpHeaders& headers() const;
	///Adds/overrides a basic header.
	void setHeader(const QByteArray& key, const QByteArray& value);

	///Returns headers for a specific file: key-value pairs
	QMap<QByteArray, QByteArray> head(QString url, const HttpHeaders& add_headers = HttpHeaders());
	///Performs GET request
	QByteArray get(QString url, const HttpHeaders& add_headers = HttpHeaders());
	///Performs PUT request
	QByteArray put(QString url, const QByteArray& data, const HttpHeaders& add_headers = HttpHeaders());
	///Performs POST request
	QByteArray post(QString url, const QByteArray& data, const HttpHeaders& add_headers = HttpHeaders());
	///Performs POST request for content type multipart
	QByteArray post(QString url, QHttpMultiPart* parts, const HttpHeaders& add_headers = HttpHeaders() );

public slots:


private:
	bool internal_;
	QNetworkProxy proxy_;
	HttpHeaders headers_;

	///Handles proxy authentification
	void handleProxyAuthentification();

	//declared away
	HttpHandler() = delete;
};

#endif // HTTPHANDLER_H
