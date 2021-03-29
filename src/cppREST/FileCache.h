#ifndef FILECACHE_H
#define FILECACHE_H

#include "cppREST_global.h"
#include <QMap>
#include <QDebug>
#include <QMutex>
#include <QDateTime>

struct CPPRESTSHARED_EXPORT StaticFile
{
	QString filename_with_path;
	QDateTime modified;
	QByteArray content;
	qint64 size;
};

class CPPRESTSHARED_EXPORT FileCache
{
public:		
	static void addFileToCache(QString id, QString filename_with_path, QByteArray content, qint64 size);
	static void removeFileFromCache(QString id);
	static QString getFileIdIfInCache(QString filename_with_path);
	static bool isInCacheAlready(QString filename_with_path);
	static StaticFile getFileById(QString id);

protected:
	FileCache();	

private:
	static FileCache& instance();
	QMutex mutex_;		
	QMap<QString, StaticFile> file_cache_;
};

#endif // FILECACHE_H
