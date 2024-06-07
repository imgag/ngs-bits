#ifndef SERVERDB_H
#define SERVERDB_H

#include "Session.h"
#include "UrlEntity.h"
#include "ClientHelper.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

class CPPRESTSHARED_EXPORT ServerDB
{
public:
    ServerDB();
    ~ServerDB();

    void initDbIfEmpty();
    void reinitializeDb();

    bool addSession(const QString string_id, const int user_id, const QString user_login, const QString user_name, const QDateTime login_time, const bool is_for_db_only);
    bool addSession(const Session new_session);
    bool addSessions(const QList<Session> all_sessions);
    bool removeSession(const QString& string_id);
    bool wipeSessions();
    bool removeSessionsOlderThan(qint64 seconds);
    Session getSession(const QString& string_id);
    QList<Session> getAllSessions();
    int getSessionsCount();

    bool addUrl(const QString string_id, const QString filename, const QString path, const QString filename_with_path, const QString file_id, const QDateTime created);
    bool addUrl(const UrlEntity new_url);
    bool addUrls(const QList<UrlEntity> all_urls);
    bool removeUrl(const QString& string_id);
    bool wipeUrls();
    bool removeUrlsOlderThan(qint64 seconds);
    bool isFileInStoreAlready(const QString& filename_with_path);
    UrlEntity getUrl(const QString& string_id);
    QList<UrlEntity> getAllUrls();
    int getUrlsCount();

protected:
    QSharedPointer<QSqlDatabase> db_;

};




#endif // SERVERDB_H
