#ifndef SERVERHELPER_H
#define SERVERHELPER_H

#include "cppREST_global.h"
#include <QCoreApplication>
#include <QDir>
#include <QUuid>
#include <QDate>
#include "Log.h"
#include "Exceptions.h"
#include "Settings.h"

class CPPRESTSHARED_EXPORT ServerHelper
{
public:
	static QString getAppName();
	static int strToInt(const QString& in);
	static bool canConvertToInt(const QString& in);
	static QString generateUniqueStr();
	static int getNumSettingsValue(const QString& key);
	static QString getStringSettingsValue(const QString& key);
	static QString getUrlWithoutParams(const QString& url);

	static bool hasBasicSettings();
	static QString getSessionBackupFileName();
	static QString getUrlStorageBackupFileName();

	static void setServerStartDateTime(QDateTime date_and_time);
	static QDateTime getServerStartDateTime();

    static QString getCurrentServerLogFile();

protected:
	ServerHelper();

private:	
	static ServerHelper& instance();
	QDateTime server_start_date_time_;


};

#endif // SERVERHELPER_H
