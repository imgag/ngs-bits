#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"
#include "Helper.h"
#include "GSvarHelper.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkProxy>
#include <QInputDialog>
#include <QApplication>
#include <QAuthenticator>
#include <QFile>
#include <QPointer>
#include <QHttpMultiPart>

HttpHandler::HttpHandler(HttpRequestHandler::ProxyType proxy_type, QObject* parent)
	: QObject(parent)
	, nmgr_()
	, proxy_type_(proxy_type)
	, headers_()
{
	//default headers
	setHeader("User-Agent", "GSvar");
	setHeader("X-Custom-User-Agent", "GSvar");

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

qint64 HttpHandler::getFileSize(QString url, const HttpHeaders& add_headers)
{
	return HttpRequestHandler(proxy_type_, this).getFileSize(url, add_headers);
}

QByteArray HttpHandler::get(QString url, const HttpHeaders& add_headers)
{
	return HttpRequestHandler(proxy_type_, this).get(url, add_headers);
}

QString HttpHandler::post(QString url, const QByteArray& data, const HttpHeaders& add_headers)
{
	return HttpRequestHandler(proxy_type_, this).post(url, data, add_headers);
}

QString HttpHandler::post(QString url, QHttpMultiPart* parts, const HttpHeaders& add_headers)
{
	return HttpRequestHandler(proxy_type_, this).post(url, parts, add_headers);
}

void HttpHandler::handleProxyAuthentification(const QNetworkProxy& proxy, QAuthenticator* auth)
{
	if (proxy_user_.isEmpty() && proxy_password_.isEmpty())
	{
		try
		{
			proxy_user_ = Settings::string("proxy_user");
			proxy_password_ = Settings::string("proxy_password");
		}
		catch(Exception& e)
		{
			proxy_user_ = QInputDialog::getText(QApplication::activeWindow(), "Proxy user required", "Proxy user for " + proxy.hostName());
			proxy_password_ = QInputDialog::getText(QApplication::activeWindow(), "Proxy password required", "Proxy password for " + proxy.hostName(), QLineEdit::Password);
		}
	}
	auth->setUser(proxy_user_);
	auth->setPassword(proxy_password_);
	nmgr_.setProxy(proxy);
}

