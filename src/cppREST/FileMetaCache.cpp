#include "FileMetaCache.h"
#include "Log.h"

FileMetaCache::FileMetaCache()
    : metadata_storage_()
{
}

FileMetaCache& FileMetaCache::instance()
{
    static FileMetaCache file_meta_cache;
    return file_meta_cache;
}

void FileMetaCache::addMetadata(FileMetadata entity)
{
    instance().metadata_storage_.insert(entity.absolute_file_path, entity);
}

void FileMetaCache::removeMetadata(QString fullname)
{
    if (instance().metadata_storage_.contains(fullname))
    {
        instance().metadata_storage_.remove(fullname);
    }
}

bool FileMetaCache::isInStorageAlready(QString absolute_file_path)
{
    QList<QString> keys = instance().metadata_storage_.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        if (instance().metadata_storage_.value(keys[i]).absolute_file_path == absolute_file_path)
        {
            return true;
        }
    }
    return false;
}

FileMetadata FileMetaCache::getMetadata(QString absolute_file_path)
{
    if (instance().metadata_storage_.contains(absolute_file_path))
    {
        return instance().metadata_storage_.value(absolute_file_path);
    }
    return FileMetadata{};
}

void FileMetaCache::removeExpiredMetadata()
{
    Log::info("Starting to cleanup cached metadata");
    QList<QString> to_be_removed {};

    QList<QString> keys = instance().metadata_storage_.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        if (instance().metadata_storage_.value(keys[i]).created.toSecsSinceEpoch() < (QDateTime::currentDateTime().toSecsSinceEpoch()-DEFAULT_CACHE_LIFETIME))
        {
            to_be_removed.append(keys[i]);
        }
    }
    for (int i = 0; i < to_be_removed.count(); ++i)
    {
        instance().metadata_storage_.remove(to_be_removed[i]);
    }

    Log::info("Number of removed metadata entities: " + QString::number(to_be_removed.length()));
}
