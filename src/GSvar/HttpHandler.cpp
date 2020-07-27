#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"
#include "Helper.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkProxy>
#include <QInputDialog>
#include <QApplication>
#include <QAuthenticator>
#include <QFile>
#include <QPointer>

HttpHandler::HttpHandler(ProxyType proxy_type, QObject* parent)
	: QObject(parent)
	, nmgr_()
{
	if (proxy_type==SYSTEM)
	{
		QNetworkProxyFactory::setUseSystemConfiguration(true);
	}
	else if (proxy_type==INI)
	{
		QNetworkProxy proxy;
		proxy.setType(QNetworkProxy::HttpProxy);
		proxy.setHostName(Settings::string("proxy_host"));
		proxy.setPort(Settings::integer("proxy_port"));
		nmgr_.setProxy(proxy);
	}
	else
	{
		nmgr_.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
	}

	connect(&nmgr_, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)), this, SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError>&)));
	connect(&nmgr_, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy& , QAuthenticator*)), this, SLOT(handleProxyAuthentification(const QNetworkProxy& , QAuthenticator*)));
}

QString HttpHandler::getHttpReply(QString url)
{
	//query
	QNetworkReply* reply = nmgr_.get(QNetworkRequest(QUrl(url)));

	//make the loop process the reply immediately
	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	//output
	QString output = reply->readAll();
	if (reply->error()!=QNetworkReply::NoError)
	{
		THROW(Exception, "Network error " + QString::number(reply->error()) + "\nError message: " + reply->errorString() + "\nReply: " + output);
	}
	reply->deleteLater();
	return output;
}

QString HttpHandler::getHttpReply(QString url, QByteArray data)
{
	//request
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("User-Agent", "GSvar");
	request.setRawHeader("X-Custom-User-Agent", "GSvar");
	request.setRawHeader("Content-Type", "application/json");
	request.setRawHeader("Content-Length", QByteArray::number(data.count()));

	//query
	QNetworkReply* reply = nmgr_.post(request, data);

	//make the loop process the reply immediately
	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	//output
	QString output = reply->readAll();
	if (reply->error()!=QNetworkReply::NoError)
	{
		THROW(Exception, "Network error " + QString::number(reply->error()) + "\nError message: " + reply->errorString() + "\nReply: " + output);
	}
	reply->deleteLater();
	return output;
}

void HttpHandler::handleSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
	foreach(const QSslError& error, errors)
	{
		qDebug() << "ignore error" << error.errorString();
	}
	reply->ignoreSslErrors(errors);
}

void HttpHandler::handleProxyAuthentification(const QNetworkProxy& proxy, QAuthenticator* auth)
{
	QString proxy_user = QInputDialog::getText(QApplication::activeWindow(), "Proxy user required", "Proxy user for " + proxy.hostName());
	auth->setUser(proxy_user);
	QString proxy_pass = QInputDialog::getText(QApplication::activeWindow(), "Proxy password required", "Proxy password for " + proxy.hostName(), QLineEdit::Password);
	auth->setPassword(proxy_pass);
}

QString HttpHandler::sendXmlFile(QString url, QString path)
{
	QPointer<QFile> file = new QFile(path, this);
	if(!file->open(QIODevice::ReadOnly))
	{
		THROW(FileParseException, "Could not open XML file " +path+ " for upload to MTB.");
	}

	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("User-Agent", "GSvar");
	request.setRawHeader("X-Custom-User-Agent", "GSvar");
	request.setRawHeader("Content-Type", "application/xml");

	//query
	QNetworkReply* reply = nmgr_.post(request, file);

	//make the loop process the reply immediately
	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	//output
	QString output = reply->readAll();
	if (reply->error()!=QNetworkReply::NoError)
	{
		THROW(Exception, "Network error " + QString::number(reply->error()) + "\nError message: " + reply->errorString() + "\nReply: " + output);
	}
	reply->deleteLater();
	return output;
}
