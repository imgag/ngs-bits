#ifndef FILEDBMANAGER_H
#define FILEDBMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QSqlRecord>
#include "Session.h"
#include "UrlEntity.h"
#include "ClientHelper.h"

class FileDbManager
{
public:
    FileDbManager(const QString path = "local_store.db");
    void initDbIfEmpty();
    void reinitializeDb();
    bool addSession(const QString string_id, const int user_id, const QString user_login, const QString user_name, const QDateTime login_time_, const bool is_for_db_only);
    bool addSession(const Session new_session);
    bool removeSession(const QString& string_id);
    Session getSession(const QString& string_id);
    QList<Session> getAllSessions();

    bool addUrl(const QString string_id, const QString filename, const QString path, const QString filename_with_path, const QString file_id, const QDateTime created);
    bool addUrl(const UrlEntity new_url);
    bool removeUrl(const QString& string_id);
    bool isFileInStoreAlrady(const QString& filename_with_path);
    UrlEntity getUrl(const QString& string_id);
    QList<UrlEntity> getAllUrls();

    ClientInfo getCurrentClientInfo();

private:
    QSqlDatabase file_database_;
};

#endif // FILEDBMANAGER_H
