#include "ServerHelper.h"
#include "Helper.h"
#include "PipelineSettings.h"
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

bool ServerHelper::settingsValid(bool test_mode, bool throw_exception_if_invalid)
{
	try
	{
		//string settings
		QStringList str_settings;
		str_settings << "server_port" << "server_host" << "ssl_certificate" << "ssl_key" << "gsvar_server_db_host" << "gsvar_server_db_name" << "gsvar_server_db_user" << "gsvar_server_db_pass";
		if (!test_mode) str_settings << "reference_genome" << "ngsd_host" << "ngsd_name" << "ngsd_user" << "ngsd_pass";
		foreach (QString entry,  str_settings)
		{
			if (ServerHelper::getStringSettingsValue(entry).isEmpty()) THROW(Exception, "String settings entry '"+entry+"' missing or empty!");
		}

		//int settings
		QStringList int_settings;
		int_settings << "url_lifetime" << "session_duration" << "gsvar_server_db_port";
		if (!test_mode) int_settings << "ngsd_port";
		foreach (QString entry,  int_settings)
		{
			if (ServerHelper::getNumSettingsValue(entry)<=0) THROW(Exception, "Integer settings entry '"+entry+"' missing or below 1!");
		}

		//load megSAP settings
		if (!test_mode)
		{
			QString megsap_settings_ini = Settings::string("megsap_settings_ini", true);
			if (megsap_settings_ini.isEmpty()) THROW(Exception, "Settings entry 'megsap_settings_ini' missing or empty!");
			PipelineSettings::loadSettings(megsap_settings_ini);

			if (PipelineSettings::rootDir().isEmpty()) THROW(Exception, "megSAP root dir not available. Settings entry 'megsap_settings_ini' is probably missing!");
            if (PipelineSettings::dataFolder().isEmpty()) THROW(Exception, "megSAP settings entry 'data_folder' is empty!");
            if (PipelineSettings::projectFolder("diagnostic").isEmpty()) THROW(Exception, "megSAP settings entry 'project_folder[diagnostic]' is empty!");
            if (PipelineSettings::projectFolder("research").isEmpty()) THROW(Exception, "megSAP settings entry 'project_folder[research]' is empty!");
            if (PipelineSettings::projectFolder("test").isEmpty()) THROW(Exception, "megSAP settings entry 'project_folder[test]' is empty!");
            if (PipelineSettings::projectFolder("external").isEmpty()) THROW(Exception, "megSAP settings entry 'project_folder[external]' is empty!");

			if (Settings::boolean("queue_update_enabled", true))
			{
                if (PipelineSettings::queueEmail().isEmpty()) THROW(Exception, "megSAP settings entry 'queue_email' is empty!");
                if (PipelineSettings::queuesDefault().isEmpty()) THROW(Exception, "megSAP settings entry 'queues_default' is empty!");
                if (PipelineSettings::queuesHighMemory().isEmpty()) THROW(Exception, "megSAP settings entry 'queues_high_mem' is empty!");
			}
		}
	}
	catch (Exception& e)
	{
		if (throw_exception_if_invalid) throw;
		else return false;
	}

	return true;
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
