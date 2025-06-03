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

struct CPPRESTSHARED_EXPORT RequestWorkerParams
{
    int socket_read_timeout;
    int socket_encryption_timeout;
    int socket_write_timeout;
};

class CPPRESTSHARED_EXPORT ServerHelper
{
public:
	static QString getAppName();
	static int strToInt(const QString& in);
	static bool canConvertToInt(const QString& in);
	static QString generateUniqueStr();	
	static QString getUrlWithoutParams(const QString& url);
	static bool settingsValid(bool test_mode, bool throw_exception_if_invalid=false);
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
