#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"
#include "Helper.h"
#include "GSvarHelper.h"
#include "ProxyDataService.h"
#include "LoginManager.h"
#include "GUIHelper.h"
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
#include <QMessageBox>


HttpHandler::HttpHandler(bool internal, QObject* parent)
	: QObject(parent)
	, nmgr_()
	, internal_(internal)
	, proxy_()
	, headers_()
{
	//default headers
	setHeader("User-Agent", "GSvar");
	setHeader("X-Custom-User-Agent", "GSvar");

	if(internal_)
	{
		//do not use proxy for internal requests
		proxy_ = QNetworkProxy(QNetworkProxy::NoProxy);
		return;
	}

	//check proxy settings
	if(!ProxyDataService::isConnected())
	{
		const QNetworkProxy& proxy = ProxyDataService::getProxy();
		if(proxy.type() != QNetworkProxy::HttpProxy)
		{
			THROW(NetworkException, "No connection to the internet! Please check your proxy settings.");
		}
		if(proxy.hostName().isEmpty() || (proxy.port() < 1))
		{
			THROW(NetworkException, "HTTP proxy without reqired host name or port provided!");
		}

		//get credentials from the user
		handleProxyAuthentification();

		//final check of the connection
		if(!ProxyDataService::isConnected())
		{
			THROW(NetworkException, "No connection to the internet! Please check your proxy settings.");
		}
	}

	//set proxy
	proxy_ = ProxyDataService::getProxy();
}

const HttpHeaders& HttpHandler::headers() const
{
	return headers_;
}

void HttpHandler::setHeader(const QByteArray& key, const QByteArray& value)
{
	headers_.insert(key, value);
}

QMap<QByteArray, QByteArray> HttpHandler::head(QString url, const HttpHeaders& add_headers)
{
    return HttpRequestHandler(proxy_, this).head(url, add_headers).headers;
}

QByteArray HttpHandler::get(QString url, const HttpHeaders& add_headers)
{
    return HttpRequestHandler(proxy_, this).get(url, add_headers).body;
}

QByteArray HttpHandler::put(QString url, const QByteArray& data, const HttpHeaders& add_headers)
{
    return HttpRequestHandler(proxy_, this).put(url, data, add_headers).body;
}

QByteArray HttpHandler::post(QString url, const QByteArray& data, const HttpHeaders& add_headers)
{
    return HttpRequestHandler(proxy_, this).post(url, data, add_headers).body;
}

QByteArray HttpHandler::post(QString url, QHttpMultiPart* parts, const HttpHeaders& add_headers)
{
    return HttpRequestHandler(proxy_, this).post(url, parts, add_headers).body;
}

void HttpHandler::handleProxyAuthentification()
{
	int max_retries = 5;
	for (int i = 0; i < max_retries; ++i)
	{
		QString user = QInputDialog::getText(GUIHelper::mainWindow(), "Proxy user required", "Proxy user", QLineEdit::Normal, Helper::userName());
		QString password = QInputDialog::getText(GUIHelper::mainWindow(), "Proxy password required", "Proxy password", QLineEdit::Password);

		if(ProxyDataService::setCredentials(user, password))
		{
			QMessageBox::information(GUIHelper::mainWindow(), "Proxy successfully configured", "The proxy was successfully configured with the provided credentials.");
			return;
		}

		QMessageBox::critical(GUIHelper::mainWindow(), "Proxy configuration failed.", QString("The provided credentials are incorrect. The proxy couldn't be configured. ") + ((i+1<max_retries)?"Please try again.":""));

	}
}

