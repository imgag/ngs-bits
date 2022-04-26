#ifndef URLMANAGER_H
#define URLMANAGER_H

#include <QMap>
#include <QMutex>
#include <QDateTime>
#include "cppREST_global.h"
#include "ServerHelper.h"

struct CPPRESTSHARED_EXPORT UrlEntity
{
	QString filename;
	QString path;
	QString filename_with_path;
	QString file_id;
	QDateTime created;
};

class CPPRESTSHARED_EXPORT UrlManager
{
public:
	static void addUrlToStorage(QString id, QString filename, QString path, QString filename_with_path);
	static void removeUrlFromStorage(const QString& id);
	static bool isInStorageAlready(const QString& filename_with_path);
	static UrlEntity getURLById(const QString& id);


public slots:
	static void removeExpiredUrls();

protected:
	UrlManager();	

private:
	static UrlManager& instance();
	QMutex mutex_;
	QMap<QString, UrlEntity> url_storage_;
};

#endif // URLMANAGER_H
