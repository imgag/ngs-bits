#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QObject>
#include <QString>
#include <QSslError>
#include <QNetworkAccessManager>
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
	HttpHandler(HttpRequestHandler::ProxyType proxy_type, QObject* parent=0);

	///Returns basic headers used for all get/post requests. Additional headers that are only used for one request can be given in the get/post methods.
	const HttpHeaders& headers() const;
	///Adds/overrides a basic header.
	void setHeader(const QByteArray& key, const QByteArray& value);

	///Return file size
	qint64 getFileSize(QString url, const HttpHeaders& add_headers = HttpHeaders());
	///Performs GET request
	QByteArray get(QString url, const HttpHeaders& add_headers = HttpHeaders());
	///Performs POST request
	QString post(QString url, const QByteArray& data, const HttpHeaders& add_headers = HttpHeaders());
	///Performs POST request for content type multipart
	QString post(QString url, QHttpMultiPart* parts, const HttpHeaders& add_headers = HttpHeaders() );

public slots:
	///Handles proxy authentification
	void handleProxyAuthentification(const QNetworkProxy& , QAuthenticator*);

private:
	QNetworkAccessManager nmgr_;
	HttpRequestHandler::ProxyType proxy_type_;
	QString proxy_user_;
	QString proxy_password_;
	HttpHeaders headers_;
	//declared away
	HttpHandler() = delete;	
};

#endif // HTTPHANDLER_H
