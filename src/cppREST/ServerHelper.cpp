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
        Log::warn("Numerical setting value unavailable: " + e.message());
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
        Log::warn("String setting value unavailable: " + e.message());
	}

	return string_value;
}

QString ServerHelper::getUrlWithoutParams(const QString& url)
{
	QList<QString> url_parts = url.split('?');
	return url_parts[0];
}

bool ServerHelper::hasMinimalSettings()
{
    return (!ServerHelper::getStringSettingsValue("server_port").isEmpty() &&
        !ServerHelper::getStringSettingsValue("server_host").isEmpty() &&
        !ServerHelper::getStringSettingsValue("ssl_certificate").isEmpty() &&
        !ServerHelper::getStringSettingsValue("ssl_key").isEmpty() &&
        (ServerHelper::getNumSettingsValue("url_lifetime")>0) &&
        (ServerHelper::getNumSettingsValue("session_duration")>0) &&
        !ServerHelper::getStringSettingsValue("gsvar_server_db_host").isEmpty() &&
        (ServerHelper::getNumSettingsValue("gsvar_server_db_port")>0) &&
        !ServerHelper::getStringSettingsValue("gsvar_server_db_name").isEmpty() &&
        !ServerHelper::getStringSettingsValue("gsvar_server_db_user").isEmpty() &&
        !ServerHelper::getStringSettingsValue("gsvar_server_db_pass").isEmpty()
    );
}

bool ServerHelper::hasProdSettings()
{
    if (!hasMinimalSettings()) return false;

    return
    (
        !ServerHelper::getStringSettingsValue("reference_genome").isEmpty() &&

        !ServerHelper::getStringSettingsValue("ngsd_host").isEmpty() &&
        (ServerHelper::getNumSettingsValue("ngsd_port")>0) &&
        !ServerHelper::getStringSettingsValue("ngsd_name").isEmpty() &&
        !ServerHelper::getStringSettingsValue("ngsd_user").isEmpty() &&
        !ServerHelper::getStringSettingsValue("ngsd_pass").isEmpty() &&

        !ServerHelper::getStringSettingsValue("projects_folder_diagnostic").isEmpty() &&
        !ServerHelper::getStringSettingsValue("projects_folder_research").isEmpty() &&
        !ServerHelper::getStringSettingsValue("projects_folder_test").isEmpty() &&
        !ServerHelper::getStringSettingsValue("projects_folder_external").isEmpty()
    );
}

QString ServerHelper::getSessionBackupFileName()
{
    QDir app_folder = QDir(QCoreApplication::applicationDirPath());
    app_folder.cdUp();
    QString session_file = app_folder.absolutePath() + QDir::separator() + QCoreApplication::applicationName() + "_sessions.txt";
    if (!QFile::exists(session_file))
    {
        Log::info("Creating a new session backup file: " + session_file);
        Helper::touchFile(session_file);
    }
    return session_file;
}

QString ServerHelper::getUrlStorageBackupFileName()
{
    QDir app_folder = QDir(QCoreApplication::applicationDirPath());
    app_folder.cdUp();
    QString url_file = app_folder.absolutePath() + QDir::separator() + QCoreApplication::applicationName() + "_urls.txt";
    if (!QFile::exists(url_file))
    {
        Log::info("Creating a new URL backup file: " + url_file);
        Helper::touchFile(url_file);
    }
    return url_file;
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
    try
    {
        Log::info("Checking log files at: " + QCoreApplication::applicationDirPath());
        QDir directory(QCoreApplication::applicationDirPath());
        QStringList logs = directory.entryList(QStringList() << "*.log" << "*.LOG", QDir::Files);

        QDateTime last_mod_time;
        QString last_mod_file;
        QString log_folder = directory.canonicalPath();
        if (!log_folder.endsWith(QDir::separator()))
        {
            log_folder = log_folder + QDir::separator();
        }
        foreach(QString filename, logs)
        {
            Log::info("Checking " +  log_folder + filename);
            if ((QFileInfo(log_folder + filename).lastModified().toSecsSinceEpoch() > last_mod_time.toSecsSinceEpoch()) || last_mod_time.isNull())
            {
                last_mod_time = QFileInfo(log_folder + filename).lastModified();
                last_mod_file = log_folder + filename;
            }
        }

        if (last_mod_file.isEmpty())
        {
            QString new_file_name = QCoreApplication::applicationFilePath().replace(".exe", "") + ".log";
            Log::warn("Could not locate the previous log file. The following file will be used: " + new_file_name);
            return new_file_name;
        }
        qint64 log_file_size = QFileInfo(last_mod_file).size();
        Log::info("Log file: " + last_mod_file + " has the size of " + QString::number(static_cast<double>(log_file_size/1024)/1024, 'f', 2) + " MB");
        // 10 MB is the max size for a log file
        if (log_file_size>(1024*1024*10))
        {
            Log::warn("Log file is too large: " + last_mod_file);
            last_mod_file = QCoreApplication::applicationFilePath().replace(".exe", "") + "_" + QDateTime().currentDateTime().toString("hh-mm-ss-dd-MM-yyyy") + ".log";
            Log::info("Suggested file name for the logs: " + last_mod_file);
        }

        if (!QFile::exists(last_mod_file)) Helper::touchFile(last_mod_file);
        Log::info("The following file will be used for logs: " + last_mod_file);
        return last_mod_file;
    }
    catch(FileAccessException& e)
    {
        Log::error("Failed to check logs: " + e.message());
        return "default.log";
    }
    catch(...)
    {
        Log::error("Unknown exception while getting the current server log file name");
        return "default.log";
    }
}

ServerHelper& ServerHelper::instance()
{
	static ServerHelper server_helper;
	return server_helper;
}
