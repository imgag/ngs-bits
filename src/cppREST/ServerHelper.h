#ifndef SERVERHELPER_H
#define SERVERHELPER_H

#include "cppREST_global.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QUuid>
#include <QDate>
#include "Exceptions.h"
#include "Settings.h"

typedef enum
{
	CRITICAL,
	FATAL,
	INFO,
	WARNING,
	DEBUG
} LoggingCategory;

class CPPRESTSHARED_EXPORT ServerHelper
{
public:
	static QString getAppName();
	static int strToInt(QString in);
	static bool canConvertToInt(QString in);
	static QString generateUniqueStr();
	static int getNumSettingsValue(QString key);
	static QString getStringSettingsValue(QString key);
	static QString getUrlWithoutParams(QString url);

	static void logger(LoggingCategory category, QString message);
	static void debug(QString message);
	static void fatal(QString message);
	static void critical(QString message);
	static void info(QString message);
	static void warning(QString message);


protected:
	ServerHelper();

private:	
	static ServerHelper& instance();


};

#endif // SERVERHELPER_H
