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
	, headers_()
{
	//default headers
	setHeader("User-Agent", "GSvar");
	setHeader("X-Custom-User-Agent", "GSvar");

	//proxy
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

	//signals+slots
	connect(&nmgr_, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)), this, SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError>&)));
	connect(&nmgr_, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy& , QAuthenticator*)), this, SLOT(handleProxyAuthentification(const QNetworkProxy& , QAuthenticator*)));
}

const HttpHeaders& HttpHandler::headers() const
{
	return headers_;
}

void HttpHandler::setHeader(const QByteArray& key, const QByteArray& value)
{
	headers_.insert(key, value);
}

QString HttpHandler::get(QString url, const HttpHeaders& add_headers)
{
	//request
	QNetworkRequest request;
	request.setUrl(url);
	for(auto it=headers_.begin(); it!=headers_.end(); ++it)
	{
		request.setRawHeader(it.key(), it.value());
	}
	for(auto it=add_headers.begin(); it!=add_headers.end(); ++it)
	{
		request.setRawHeader(it.key(), it.value());
	}

	//query
	QNetworkReply* reply = nmgr_.get(request);

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

QString HttpHandler::post(QString url, const QByteArray& data, const HttpHeaders& add_headers)
{
	//request
	QNetworkRequest request;
	request.setUrl(url);
	for(auto it=headers_.begin(); it!=headers_.end(); ++it)
	{
		request.setRawHeader(it.key(), it.value());
	}
	for(auto it=add_headers.begin(); it!=add_headers.end(); ++it)
	{
		request.setRawHeader(it.key(), it.value());
	}

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

