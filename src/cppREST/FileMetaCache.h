#ifndef FILEMETACACHE_H
#define FILEMETACACHE_H


#include <QDateTime>
#include "cppREST_global.h"
#include "ThreadSafeHashMap.h"

struct CPPRESTSHARED_EXPORT FileMetadata
{
    QString absolute_file_path; // full name with the path
    QString absolute_path; // path without filename
    QString filename; // only filename (without path)
    bool has_size_info;
    qint64 size;
    bool has_existence_info;
    bool file_exists;
    QDateTime modified;
    QDateTime created;

    FileMetadata()
        : absolute_file_path()
        , absolute_path()
        , filename()
        , has_size_info()
        , size()
        , has_existence_info()
        , file_exists()
        , modified()
        , created()
    {
    }

    FileMetadata(const QString absolute_file_path, const QString absolute_path, const QString filename, const bool has_size_info, const qint64 size, const bool has_existence_info, const bool file_exists, const QDateTime modified, const QDateTime created)
        : absolute_file_path(absolute_file_path)
        , absolute_path(absolute_path)
        , filename(filename)
        , has_size_info(has_size_info)
        , size(size)
        , has_existence_info(has_existence_info)
        , file_exists(file_exists)
        , modified(modified)
        , created(created)
    {
    }

    bool isEmpty()
    {
        return absolute_file_path.isEmpty() && absolute_path.isEmpty() && filename.isEmpty() && created.isNull();
    }
};

class CPPRESTSHARED_EXPORT FileMetaCache
{
public:
    static const qint64 DEFAULT_CACHE_LIFETIME = 60*20; // in seconds
    static void addMetadata(FileMetadata entity);
    static void removeMetadata(QString fullname);
    static bool isInStorageAlready(QString absolute_file_path);
    static FileMetadata getMetadata(QString absolute_file_path);
    static void removeExpiredMetadata();

protected:
    FileMetaCache();

private:
    static FileMetaCache& instance();
    ThreadSafeHashMap<QString, FileMetadata> metadata_storage_;
};




#endif // FILEMETACACHE_H
