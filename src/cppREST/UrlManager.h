#ifndef URLMANAGER_H
#define URLMANAGER_H

#include <QDateTime>
#include "cppREST_global.h"
#include "ServerHelper.h"
#include "ServerDB.h"
#include "ThreadSafeHashMap.h"

class CPPRESTSHARED_EXPORT UrlManager
{
public:
    static const qint64 DEFAULT_URL_LIFETIME = 600; // in seconds
    static void addNewUrl(UrlEntity url_entity);
    static void removeUrl(QString id);
    static bool isInStorageAlready(QString filename_with_path);
    static UrlEntity getURLById(QString id);
    static QList<UrlEntity> getAllUrls();
    static bool isValidUrl(QString token);
    static void removeExpiredUrls();

protected:
	UrlManager();	

private:
    static UrlManager& instance();
    ThreadSafeHashMap<QString, UrlEntity> url_storage_;
};

#endif // URLMANAGER_H
