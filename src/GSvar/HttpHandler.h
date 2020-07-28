#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QObject>
#include <QString>
#include <QSslError>
#include <QNetworkAccessManager>

using HttpHeaders = QMap<QByteArray, QByteArray>;

///Helper class for HTTP(S) communication with webserver
class HttpHandler
		: public QObject
{
	Q_OBJECT

public:
	///Proxy type
	enum ProxyType
	{
		SYSTEM, //from system settings
		INI, //from ini file
		NONE //no proxy
	};

	///Constructor
	HttpHandler(ProxyType proxy_type, QObject* parent=0);

	///Returns basic headers used for all get/post requests. Additional headers that are only used for one request can be given in the get/post methods.
	const HttpHeaders& headers() const;
	///Adds/overrides a basic header.
	void setHeader(const QByteArray& key, const QByteArray& value);

	///Performs GET request
	QString get(QString url, const HttpHeaders& add_headers = HttpHeaders());
	///Performs POST request
	QString post(QString url, const QByteArray& data, const HttpHeaders& add_headers = HttpHeaders());

private slots:
	///Handles SSL errors (by ignoring them)
	void handleSslErrors(QNetworkReply*, const QList<QSslError>&);
	///Handles proxy authentification
	void handleProxyAuthentification(const QNetworkProxy& , QAuthenticator*);

private:
	QNetworkAccessManager nmgr_;
	HttpHeaders headers_;
	//declared away
	HttpHandler() = delete;
};

#endif // HTTPHANDLER_H
