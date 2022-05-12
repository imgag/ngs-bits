#include "ServerHelper.h"

ServerHelper::ServerHelper()
{
}

QString ServerHelper::getAppName()
{
	return QCoreApplication::applicationName();
}

int ServerHelper::strToInt(const QString& in)
{
	bool ok;
	int dec = in.toInt(&ok, 10);
	if (!ok) THROW(ArgumentException, "Could not convert string to integer");

	return dec;
}

bool ServerHelper::canConvertToInt(const QString& in)
{
	bool ok;
	in.toInt(&ok, 10);
	return ok;
}

QString ServerHelper::generateUniqueStr()
{
	return QUuid::createUuid().toString().replace("{", "").replace("}", "");
}

int ServerHelper::getNumSettingsValue(const QString& key)
{
	int num_value = 0;
	try
	{
		 num_value = Settings::integer(key);
	}
	catch (Exception& e)
	{
		Log::warn("Numerical settings value unavailable: " + e.message());
	}

	return num_value;
}

QString ServerHelper::getStringSettingsValue(const QString& key)
{
	QString string_value = "";
	try
	{
		 string_value = Settings::string(key);
	}
	catch (Exception& e)
	{
		Log::warn("String settings value unavailable: " + e.message());
	}

	return string_value;
}

QString ServerHelper::getUrlWithoutParams(const QString& url)
{
	QList<QString> url_parts = url.split('?');
	return url_parts[0];
}

QString ServerHelper::getUrlProtocol(const bool& return_http)
{
	if (return_http) return "http://";
	return "https://";
}

QString ServerHelper::getUrlPort(const bool& return_http)
{
	QString port = "";
	if (return_http) port = ServerHelper::getStringSettingsValue("http_server_port");
	if (!return_http) port = ServerHelper::getStringSettingsValue("https_server_port");
	if (!port.isEmpty()) port = ":" + port;
	return port;
}

bool ServerHelper::hasBasicSettings()
{
	if (!ServerHelper::getStringSettingsValue("https_server_port").isEmpty() &&
		!ServerHelper::getStringSettingsValue("http_server_port").isEmpty() &&
		!ServerHelper::getStringSettingsValue("server_host").isEmpty() &&
		!ServerHelper::getStringSettingsValue("ssl_certificate").isEmpty() &&
		!ServerHelper::getStringSettingsValue("ssl_key").isEmpty() &&
		(ServerHelper::getNumSettingsValue("url_lifetime")>0) &&
		(ServerHelper::getNumSettingsValue("session_duration")>0))
	{
		return true;
	}
	return false;
}

QString ServerHelper::getServerUrl(const bool& return_http)
{
	return getUrlProtocol(return_http) + ServerHelper::getStringSettingsValue("server_host") +
			ServerHelper::getUrlPort(return_http);
}

ServerHelper& ServerHelper::instance()
{
	static ServerHelper server_helper;
	return server_helper;
}
