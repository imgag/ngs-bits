#include "ServerHelper.h"
#include <QStandardPaths>
#include <QDir>

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

bool ServerHelper::hasBasicSettings()
{
	if (!ServerHelper::getStringSettingsValue("server_port").isEmpty() &&
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

QString ServerHelper::getSessionBackupFileName()
{
    return QCoreApplication::applicationDirPath() + QDir::separator() + QCoreApplication::applicationName() + "_sessions.txt";
}

QString ServerHelper::getUrlStorageBackupFileName()
{
    return QCoreApplication::applicationDirPath() + QDir::separator() + QCoreApplication::applicationName() + "_urls.txt";
}

void ServerHelper::setServerStartDateTime(QDateTime date_and_time)
{
	instance().server_start_date_time_ = date_and_time;
}

QDateTime ServerHelper::getServerStartDateTime()
{
	return instance().server_start_date_time_;
}

ServerHelper& ServerHelper::instance()
{
	static ServerHelper server_helper;
	return server_helper;
}

QString ServerHelper::getStandardFileLocation()
{
	QString path = QDir::tempPath();
	QStringList default_paths = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
	if(default_paths.isEmpty())
	{
		Log::warn("No local application data path was found!");
	}
	else
	{
		path = default_paths[0];
	}
	if (!QDir().exists(path)) return "";

	if (!path.endsWith(QDir::separator())) path = path + QDir::separator();
	return path;
}
