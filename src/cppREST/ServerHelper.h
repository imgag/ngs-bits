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
	static QString getUrlProtocol(const bool& return_http);

protected:
	ServerHelper();

private:	
	static ServerHelper& instance();


};

#endif // SERVERHELPER_H
