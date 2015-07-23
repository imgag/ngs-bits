#include "HttpHandler.h"
#include "Exceptions.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

HttpHandler::HttpHandler(QObject* parent)
	: QObject(parent)
{
}

QString HttpHandler::getHttpReply(QString url)
{
	//create asynchronous reply
	QNetworkAccessManager mngr;

	//get reply
	QNetworkReply* reply = mngr.get(QNetworkRequest(QUrl(url)));
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
	return reply->readAll();
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
