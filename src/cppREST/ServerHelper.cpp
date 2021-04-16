#include "ServerHelper.h"

ServerHelper::ServerHelper()
{
}

QString ServerHelper::getAppName()
{
	return QCoreApplication::applicationName();
}

int ServerHelper::strToInt(QString in)
{
	bool ok;
	int dec = in.toInt(&ok, 10);
	if (!ok) THROW(ArgumentException, "Could not convert string to integer");

	return dec;
}

bool ServerHelper::canConvertToInt(QString in)
{
	bool ok;
	in.toInt(&ok, 10);
	return ok;
}

QString ServerHelper::generateUniqueStr()
{
	return QUuid::createUuid().toString().replace("{", "").replace("}", "");
}

int ServerHelper::getNumSettingsValue(QString key)
{
	int num_value = 0;
	try
	{
		 num_value = Settings::integer(key);
	}
	catch (Exception& e)
	{
		debug("The server may crash due to the missing numerical key " + key + " from settings: " + e.message());
	}

	return num_value;
}

QString ServerHelper::getStringSettingsValue(QString key)
{
	QString string_value = "";
	try
	{
		 string_value = Settings::string(key);
	}
	catch (Exception& e)
	{
		debug("The server may crash due to the missing string key " + key + " from settings: " + e.message());
	}

	return string_value;
}

QString ServerHelper::getUrlWithoutParams(QString url)
{
	QList<QString> url_parts = url.split('?');
	return url_parts[0];
}

void ServerHelper::logger(LoggingCategory category, QString message)
{
	QString time_stamp = QDate::currentDate().toString("dd/MM/yyyy") + " " + QTime::currentTime().toString("hh:mm:ss:zzz");
	QString log_statement = "";
	int msg_level = 0;
	switch (category) {
		case LoggingCategory::CRITICAL:
			msg_level = 0;
			log_statement = QString("%1 - [Critical] %2").arg(time_stamp, message);
			break;
		case LoggingCategory::FATAL:
			msg_level = 0;
			log_statement = QString("%1 - [Fatal] %2").arg(time_stamp, message);
			break;
		case LoggingCategory::INFO:
			msg_level = 1;
			log_statement = QString("%1 - [Info] %2").arg(time_stamp, message);
			break;
		case LoggingCategory::WARNING:
			msg_level = 2;
			log_statement = QString("%1 - [Warning] %2").arg(time_stamp, message);
			break;
		case LoggingCategory::DEBUG:
		default:
			msg_level = 3;
			log_statement = QString("%1 - [Debug] %2").arg(time_stamp, message);
	}

	QTextStream out_stream(stdout);
	out_stream.setCodec("UTF-8");
	out_stream.setGenerateByteOrderMark(false);
	out_stream << log_statement << endl;
}

void ServerHelper::debug(QString message)
{
	logger(LoggingCategory::DEBUG, message);
}

void ServerHelper::fatal(QString message)
{
	logger(LoggingCategory::FATAL, message);
}

void ServerHelper::critical(QString message)
{
	logger(LoggingCategory::CRITICAL, message);
}

void ServerHelper::info(QString message)
{
	logger(LoggingCategory::INFO, message);
}

void ServerHelper::warning(QString message)
{
	logger(LoggingCategory::WARNING, message);
}

ServerHelper& ServerHelper::instance()
{
	static ServerHelper server_helper;
	return server_helper;
}
