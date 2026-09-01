#include "FastFileInfo.h"
#include "FileMetaCache.h"
#include <QFileInfo>
#include "Settings.h"

FastFileInfo::FastFileInfo(const QString& absolute_file_path)
    : absolute_file_path_(absolute_file_path)
{
	if (FileMetaCache::isInStorageAlready(absolute_file_path_))
	{
		FileMetadata meta_cache = FileMetaCache::getMetadata(absolute_file_path_);
		absolute_path_ = meta_cache.absolute_path;
		filename_ = meta_cache.filename;
		size_ = meta_cache.size;
		exists_ = meta_cache.file_exists;
		last_modified_ = meta_cache.modified;
	}
	else
	{
		QFileInfo info = QFileInfo(absolute_file_path_);
		absolute_path_ = info.absolutePath();
		filename_ = info.fileName();
		size_ = 0;
		exists_ = false;
		last_modified_ = info.lastModified();
		if (cachingEnabled()) FileMetaCache::addMetadata(FileMetadata(absolute_file_path_, absolute_path_, filename_, false, size_, false, exists_, last_modified_, QDateTime::currentDateTime()));
	}
}

qint64 FastFileInfo::size() const
{
	FileMetadata cached_info = FileMetaCache::getMetadata(absolute_file_path_);
	if (cached_info.has_size_info) return cached_info.size;

	QFileInfo file_info = QFileInfo(absolute_file_path_);
	cached_info.size = file_info.size();
	cached_info.has_size_info = true;

	if (cachingEnabled()) FileMetaCache::addMetadata(cached_info);

	return  cached_info.size;
}

bool FastFileInfo::exists() const
{
	FileMetadata cached_info = FileMetaCache::getMetadata(absolute_file_path_);
	if (cached_info.has_existence_info) return cached_info.file_exists;

	QFileInfo file_info = QFileInfo(absolute_file_path_);
	cached_info.file_exists = file_info.exists();
	cached_info.has_existence_info = true;

	if (cachingEnabled()) FileMetaCache::addMetadata(cached_info);

	return  cached_info.file_exists;
}

QString FastFileInfo::absoluteFilePath() const
{
	return absolute_file_path_;
}

QString FastFileInfo::absolutePath() const
{
	return absolute_path_;
}

QString FastFileInfo::fileName() const
{
	return filename_;
}

QDateTime FastFileInfo::lastModified() const
{
	return last_modified_;
}

bool FastFileInfo::cachingEnabled()
{
	static const bool enabled = Settings::boolean("enable_file_metadata_caching", true);
	return enabled;
}