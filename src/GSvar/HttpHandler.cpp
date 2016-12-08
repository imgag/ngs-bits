#include "HttpHandler.h"
#include "Exceptions.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkProxy>

HttpHandler::HttpHandler(QObject* parent)
	: QObject(parent)
{
}

QString HttpHandler::getHttpReply(QString url)
{
	QNetworkAccessManager nmgr;

	//query
	QNetworkReply* reply = nmgr.get(QNetworkRequest(QUrl(url)));
	/* This would be the correct solution to handle the self-signed certificate, but is not used because of the high maintenance effort:
	 * (1) certificate file needs to be renewed each year
	 * (2) certificate is based on IP, which may change
	 * QFile cert_file("://Resources/SRV017.crt");
	 * cert_file.open(QIODevice::ReadOnly);
	 * QSslCertificate certificate(&cert_file);
	 * reply->ignoreSslErrors(QList<QSslError>() << QSslError(QSslError::SelfSignedCertificate, certificate));
	 */
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(handleSslErrors(QList<QSslError>)));

	//make the loop process the reply immediately
	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	//handle errors
	if (reply->error()!=QNetworkReply::NoError)
	{
		THROW(Exception, "Network error " + QString::number(reply->error()) + ". Error message: " + reply->errorString());
	}

	//return result
	QByteArray output = reply->readAll();
	reply->deleteLater();
	return output;
}

QString HttpHandler::getHttpReply(QString url, QByteArray data)
{
	QNetworkAccessManager nmgr;

	//TODO: get proxy settings from INI file (except username/password) and make it optional
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::HttpProxy);
	proxy.setHostName("httpproxy.zit.med.uni-tuebingen.de");
	proxy.setPort(88);
	proxy.setUser("ahsturm1");
	proxy.setPassword("sPW005?!");
	nmgr.setProxy(proxy);

	//request
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("User-Agent", "GSvar");
	request.setRawHeader("X-Custom-User-Agent", "GSvar");
	request.setRawHeader("Content-Type", "application/json");
	request.setRawHeader("Content-Length", QByteArray::number(data.count()));

	//query
	QNetworkReply* reply = nmgr.post(request, data);
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(handleSslErrors(QList<QSslError>)));

	//make the loop process the reply immediately
	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	QString output = reply->readAll();

	//handle errors
	if (reply->error()!=QNetworkReply::NoError)
	{
		THROW(Exception, "Network error " + QString::number(reply->error()) + "\nError message: " + reply->errorString() + "\nReply: " + output);
	}

	//return result
	reply->deleteLater();
	return output;
}

void HttpHandler::handleSslErrors(QList<QSslError> errors)
{
	/*
		foreach(QSslError error, errors)
		{
			qDebug() << "IGNORED SSL ERROR - type:" << error.error() << " string:" << error.errorString();
			Log::warn("Ignoring SSL error " + QString::number(error.error()) + ". Error messge: " + error.errorString());
		}
	*/
	qobject_cast<QNetworkReply*>(sender())->ignoreSslErrors(errors);
}
