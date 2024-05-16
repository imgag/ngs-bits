#ifndef URLMANAGER_H
#define URLMANAGER_H

#include <QDateTime>
#include "cppREST_global.h"
#include "ServerHelper.h"
#include "FileDbManager.h"

class CPPRESTSHARED_EXPORT UrlManager
{
public:
    static const qint64 DEFAULT_URL_LIFETIME = 600; // in seconds
    static void addNewUrl(UrlEntity url_entity);
	static void removeUrl(const QString& id);
	static bool isInStorageAlready(const QString& filename_with_path);
	static UrlEntity getURLById(const QString& id);
    static bool isValidUrl(QString token);
    static int removeExpiredUrls();

protected:
	UrlManager();	

private:
    static UrlManager& instance();
};

#endif // URLMANAGER_H
