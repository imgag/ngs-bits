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
		qDebug() << "Cannot find numerical key " + key + " in the settings: " + e.message();
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
		qDebug() << "Cannot find string key " + key + " in the settings: " + e.message();
	}

	return string_value;
}

QString ServerHelper::getUrlWithoutParams(QString url)
{
	QList<QString> url_parts = url.split('?');
	return url_parts[0];
}


ServerHelper& ServerHelper::instance()
{
	static ServerHelper server_helper;
	return server_helper;
}
