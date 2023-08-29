#include "ServerHelper.h"
#include "Helper.h"
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

QString ServerHelper::getCurrentServerLogFile()
{
    QDir directory(QCoreApplication::applicationDirPath());
    QStringList logs = directory.entryList(QStringList() << "*.log" << "*.LOG", QDir::Files);

    QDateTime last_mod_time;
    QString last_mod_file;
    foreach(QString filename, logs)
    {
        if (QFileInfo(filename).lastModified() > last_mod_time)
        {
            last_mod_time = QFileInfo(filename).lastModified();
            last_mod_file = filename;
        }
    }

    if (last_mod_file.isEmpty()) return QCoreApplication::applicationFilePath().replace(".exe", "") + ".log";
    if (QFileInfo(last_mod_file).size()>(1024*1024*50))
    {
        QList<QString> name_items = last_mod_file.split(".");
        if (name_items.size()>1)
        {
            name_items[name_items.count()-2] = name_items[name_items.count()-2] + "_" + QDateTime().currentDateTime().toString("hh-mm-ss-dd-MM-yyyy");
            last_mod_file = name_items.join(".");
        }
    }

    if (!QCoreApplication::applicationDirPath().endsWith(QDir::separator()))
    {
        last_mod_file = QDir::separator() + last_mod_file;
    }

    last_mod_file = QCoreApplication::applicationDirPath() + last_mod_file;
    if (!QFile::exists(last_mod_file)) Helper::touchFile(last_mod_file);

    return last_mod_file;
}

ServerHelper& ServerHelper::instance()
{
	static ServerHelper server_helper;
	return server_helper;
}
