#include "ClientHelper.h"

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

	return filename.endsWith(".bam", Qt::CaseInsensitive);
}

QString ClientHelper::stripSecureToken(QString url)
{
	int token_pos = url.indexOf("?token", Qt::CaseInsensitive);
	if (token_pos > -1) url = url.left(token_pos);
	return  url;
}

ServerInfo ClientHelper::getServerInfo()
{
	ServerInfo info;
	QByteArray response;
	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
	try
	{
		response = HttpRequestHandler(HttpRequestHandler::ProxyType::NONE).get(serverApiUrl()+ "info", add_headers);
	}
	catch (Exception& e)
	{
		Log::error("Server availability problem: " + e.message());
		return info;
	}

	if (response.isEmpty())
	{
		Log::error("Could not parse the server response. The application will be closed");
		return info;
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(response);
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
	QByteArray response;
	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
	try
	{
		response = HttpRequestHandler(HttpRequestHandler::ProxyType::NONE).get(serverApiUrl()+ "current_client", add_headers);
	}
	catch (Exception& e)
	{
		Log::error("Could not get the client version information from the server: " + e.message());
		return info;
	}

	if (response.isEmpty())
	{
		Log::error("Could not parse the server response");
		return info;
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(response);
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
	QByteArray response;
	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
	try
	{
		response = HttpRequestHandler(HttpRequestHandler::ProxyType::NONE).get(serverApiUrl()+ "notification", add_headers);
	}
	catch (Exception& e)
	{
		Log::error("Could not get any user notifications from the server: " + e.message());
		return user_notification;
	}

	if (response.isEmpty())
	{
		Log::error("Could not parse the server response");
		return user_notification;
	}

	QJsonDocument json_doc = QJsonDocument::fromJson(response);
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
