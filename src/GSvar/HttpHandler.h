#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QObject>
#include <QString>
#include <QSslError>
#include <QNetworkAccessManager>

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
	///Handles request (GET)
	QString getHttpReply(QString url);
	///Handles request (POST)
	QString getHttpReply(QString url, QByteArray data);

private slots:
	///Handles SSL errors (by ignoring them)
	void handleSslErrors(QNetworkReply*, const QList<QSslError>&);
	///Handles proxy authentification
	void handleProxyAuthentification(const QNetworkProxy& , QAuthenticator*);

private:
	QNetworkAccessManager nmgr_;

	//declared away
	HttpHandler();
};

#endif // HTTPHANDLER_H
