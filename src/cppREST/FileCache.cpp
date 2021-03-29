#include "FileCache.h"

FileCache::FileCache()
	: file_cache_()
{
}

FileCache& FileCache::instance()
{
	static FileCache file_cache;
	return file_cache;
}

void FileCache::addFileToCache(QString id, QString filename_with_path, QByteArray content, qint64 size)
{
	instance().mutex_.lock();
	instance().file_cache_.insert(id, StaticFile{filename_with_path, QDateTime::currentDateTime(), content, size});
	instance().mutex_.unlock();
}

void FileCache::removeFileFromCache(QString id)
{
	if (instance().file_cache_.contains(id))
	{
		instance().mutex_.lock();
		instance().file_cache_.remove(id);
		instance().mutex_.unlock();
	}
}

QString FileCache::getFileIdIfInCache(QString filename_with_path)
{
	QMapIterator<QString, StaticFile> i(instance().file_cache_);
	while (i.hasNext()) {
		i.next();
		qDebug() << i.key() << ": " << i.value().filename_with_path;
		if (i.value().filename_with_path == filename_with_path)
		{
			return i.key();
		}
	}

	return "";
}

bool FileCache::isInCacheAlready(QString filename_with_path)
{
	QMapIterator<QString, StaticFile> i(instance().file_cache_);
	while (i.hasNext()) {
		i.next();
		qDebug() << i.key() << ": " << i.value().filename_with_path;
		if (i.value().filename_with_path == filename_with_path)
		{
			return true;
		}
	}

	return false;
}

StaticFile FileCache::getFileById(QString id)
{
	if (instance().file_cache_.contains(id))
	{
		return instance().file_cache_[id];
	}
	return StaticFile{};
}
