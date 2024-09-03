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
        size_ = info.size();
        exists_ = info.exists();
        FileMetaCache::addMetadata(FileMetadata(filename_, absolute_path_, filename_, true, size_, true, exists_, QDateTime::currentDateTime()));
    }
}

qint64 FastFileInfo::size()
{
    return size_;
}

bool FastFileInfo::exists()
{
    return exists_;
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
