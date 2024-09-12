#include "FastFileInfo.h"

FastFileInfo::FastFileInfo(QString absolute_file_path)
    : absolute_file_path_(absolute_file_path)
{
    if (FileMetaCache::isInStorageAlready(absolute_file_path_))
    {
        FileMetadata meta_cache = FileMetaCache::getMetadata(filename_);
        absolute_path_ = meta_cache.absolute_path;
        filename_ = meta_cache.filename;
        size_ = meta_cache.size;
        exists_ = meta_cache.file_exists;
    }
    else
    {
        QFileInfo info = QFileInfo(absolute_file_path_);
        absolute_path_ = info.absolutePath();
        filename_ = info.fileName();
        size_ = 0;
        exists_ = false;
        FileMetaCache::addMetadata(FileMetadata(filename_, absolute_path_, filename_, false, size_, false, exists_, QDateTime::currentDateTime()));
    }
}

qint64 FastFileInfo::size()
{
    FileMetadata cached_info = FileMetaCache::getMetadata(absolute_file_path_);
    if (cached_info.has_size_info)
    {
        return cached_info.size;
    }

    QFileInfo file_info = QFileInfo(absolute_file_path_);
    cached_info.size = file_info.size();
    cached_info.has_size_info = true;

    FileMetaCache::addMetadata(cached_info);
    return  cached_info.size;
}

bool FastFileInfo::exists()
{
    FileMetadata cached_info = FileMetaCache::getMetadata(absolute_file_path_);
    if (cached_info.has_existence_info)
    {
        return cached_info.file_exists;
    }

    QFileInfo file_info = QFileInfo(absolute_file_path_);
    cached_info.file_exists = file_info.exists();
    cached_info.has_existence_info = true;

    FileMetaCache::addMetadata(cached_info);
    return  cached_info.file_exists;
}

QString FastFileInfo::absoluteFilePath()
{
    return absolute_file_path_;
}

QString FastFileInfo::absolutePath()
{
    return absolute_path_;
}

QString FastFileInfo::fileName()
{
    return filename_;
}
