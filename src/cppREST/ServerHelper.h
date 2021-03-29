#ifndef SERVERHELPER_H
#define SERVERHELPER_H

#include "cppREST_global.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QUuid>
#include "Exceptions.h"
#include "Settings.h"

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

protected:
	ServerHelper();

private:	
	static ServerHelper& instance();

};

#endif // SERVERHELPER_H
