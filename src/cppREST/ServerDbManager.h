#ifndef SERVERDBMANAGER_H
#define SERVERDBMANAGER_H

#include "cppREST_global.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QSqlRecord>
#include "Session.h"
#include "UrlEntity.h"
#include "ClientHelper.h"

class CPPRESTSHARED_EXPORT ServerDbManager
{
public:
    static void initDbIfEmpty();
    static void reinitializeDb();
    static bool addSession(const QString string_id, const int user_id, const QString user_login, const QString user_name, const QDateTime login_time, const bool is_for_db_only);
    static bool addSession(const Session new_session);
    static bool removeSession(const QString& string_id);
    static bool removeSessionsOlderThan(qint64 seconds);
    static Session getSession(const QString& string_id);
    static QList<Session> getAllSessions();
    static int getSessionsCount();

    static bool addUrl(const QString string_id, const QString filename, const QString path, const QString filename_with_path, const QString file_id, const QDateTime created);
    static bool addUrl(const UrlEntity new_url);
    static bool removeUrl(const QString& string_id);
    static bool removeUrlsOlderThan(qint64 seconds);
    static bool isFileInStoreAlready(const QString& filename_with_path);
    static UrlEntity getUrl(const QString& string_id);
    static QList<UrlEntity> getAllUrls();
    static int getUrlsCount();

protected:
    ServerDbManager();
    ~ServerDbManager();

private:
    static ServerDbManager& instance();
    static void openConnectionIfClosed();
    QSqlDatabase server_database_;
};



#endif // SERVERDBMANAGER_H
