#include "ClientHelper.h"

#include <QNetworkProxy>

bool ClientHelper::isClientServerMode()
{
	return !Settings::string("server_host", true).trimmed().isEmpty() && !Settings::string("server_port", true).trimmed().isEmpty();
}

bool ClientHelper::isRunningOnServer()
{
	return !Settings::string("ssl_certificate",true).trimmed().isEmpty() && !Settings::string("ssl_key",true).trimmed().isEmpty();
}

bool ClientHelper::isBamFile(QString filename)
{
	if (Helper::isHttpUrl(filename))
	{
		filename = QUrl(filename).toString(QUrl::RemoveQuery);
	}

	return filename.endsWith(".bam", Qt::CaseInsensitive) ||  filename.endsWith(".cram", Qt::CaseInsensitive);
}

QString ClientHelper::stripSecureToken(QString url)
{
	int token_pos = url.indexOf("?token", Qt::CaseInsensitive);
	if (token_pos > -1) url = url.left(token_pos);
	return  url;
}

ServerInfo ClientHelper::getServerInfo(int& status_code)
{
	ServerInfo info;
    ServerReply reply;
	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
    add_headers.insert("Content-type", "application/json");
	try
	{
        reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(serverApiUrl()+ "info", add_headers);
        status_code = reply.status_code;
	}
    catch (HttpException& e)
	{
		Log::error("Server availability problem: " + e.message());
		return info;
	}

    if (reply.body.isEmpty())
	{
		Log::error("Could not parse the server response. The application will be closed");
		return info;
	}

    QJsonDocument json_doc = QJsonDocument::fromJson(reply.body);
	if (json_doc.isObject())
	{
		if (json_doc.object().contains("version")) info.version = json_doc.object()["version"].toString();
		if (json_doc.object().contains("api_version")) info.api_version = json_doc.object()["api_version"].toString();
		if (json_doc.object().contains("start_time")) info.server_start_time = QDateTime::fromSecsSinceEpoch(json_doc.object()["start_time"].toInt());
	}

	return info;
}

ClientInfo ClientHelper::getClientInfo()
{
	ClientInfo info;
    ServerReply reply;
	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
    add_headers.insert("Content-type", "application/json");
	try
	{
        reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(serverApiUrl()+ "current_client", add_headers);
	}
    catch (HttpException& e)
	{
		Log::error("Could not get the client version information from the server: " + e.message());
		return info;
	}

    if (reply.body.isEmpty())
	{
		Log::error("Could not parse the server response");
		return info;
	}

    QJsonDocument json_doc = QJsonDocument::fromJson(reply.body);
	if (json_doc.isObject())
	{
		if (json_doc.object().contains("version")) info.version = json_doc.object()["version"].toString();
		if (json_doc.object().contains("message")) info.message = json_doc.object()["message"].toString();
		if (json_doc.object().contains("date")) info.date = QDateTime::fromSecsSinceEpoch(json_doc.object()["date"].toInt());
	}
	return info;
}

UserNotification ClientHelper::getUserNotification()
{
	UserNotification user_notification;
    ServerReply reply;
	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
    add_headers.insert("Content-type", "application/json");
	try
	{
        reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(serverApiUrl()+ "notification", add_headers);
	}
    catch (HttpException& e)
	{
		Log::error("Could not get any user notifications from the server: " + e.message());
		return user_notification;
	}

    if (reply.body.isEmpty())
	{
		Log::error("Could not parse the server response");
		return user_notification;
	}

    QJsonDocument json_doc = QJsonDocument::fromJson(reply.body);
	if (json_doc.isObject())
	{
		if (json_doc.object().contains("id")) user_notification.id = json_doc.object()["id"].toString();
		if (json_doc.object().contains("message")) user_notification.message = json_doc.object()["message"].toString();
	}
	return user_notification;
}

QString ClientHelper::serverApiVersion()
{
	return "v1";
}

QString ClientHelper::serverApiUrl()
{
	return  "https://" + Settings::string("server_host", true) + ":" + Settings::string("server_port", true) + "/" + serverApiVersion() + "/";
}
