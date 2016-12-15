#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkProxy>
#include <QInputDialog>
#include <QApplication>
#include <QAuthenticator>

HttpHandler::HttpHandler(QObject* parent)
	: QObject(parent)
	, nmgr_()
{
	QNetworkProxyFactory::setUseSystemConfiguration(true);

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

/* Not used - we use system proxy settings with credentials cache
void HttpHandler::proxyFromSettings(QNetworkAccessManager& nmgr)
{
	//no proxy set => return
	QString proxy_host = Settings::string("proxy_host");
	if (proxy_host.isEmpty()) return;

	//create proxy
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::HttpProxy);
	proxy.setHostName(proxy_host);
	int proxy_port = Settings::integer("proxy_port");
	proxy.setPort(proxy_port);

	//set proxy user/password
	QString proxy_user = Settings::string("proxy_user");
	if (!proxy_user.isEmpty())
	{
		proxy.setUser(proxy_user);
		QString proxy_password = Settings::string("proxy_password");
		if (proxy_password=="ASK")
		{
			proxy_password = QInputDialog::getText(QApplication::activeWindow(), "Proxy password required", "Password for user " + proxy_user);
		}
		proxy.setPassword(proxy_password);
	}

	//set proxy
	nmgr.setProxy(proxy);
}
*/
