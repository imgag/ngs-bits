#ifndef URLENTITY_H
#define URLENTITY_H

#include "cppREST_global.h"
#include <QDateTime>

struct CPPRESTSHARED_EXPORT UrlEntity
{
    QString string_id;
    QString filename;
    QString path;
    QString filename_with_path;
    QString file_id;
    QDateTime created;

    UrlEntity()
        : string_id()
        , filename()
        , path()
        , filename_with_path()
        , file_id()
        , created()
    {
    }

    UrlEntity(const QString string_id_in, const QString filename_in, const QString path_in, const QString filename_with_path_in, const QString file_id_in, const QDateTime created_in)
        : string_id(string_id_in)
        , filename(filename_in)
        , path(path_in)
        , filename_with_path(filename_with_path_in)
        , file_id(file_id_in)
        , created(created_in)
    {
    }

    bool isEmpty()
    {
        return string_id.isEmpty() && filename.isEmpty() && created.isNull();
    }
};

#endif // URLENTITY_H
