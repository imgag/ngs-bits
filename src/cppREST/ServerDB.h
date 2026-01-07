#ifndef SERVERDB_H
#define SERVERDB_H

#include "Session.h"
#include "UrlEntity.h"
#include <QSqlDatabase>
#include <QJsonDocument>
#include <QSharedPointer>

class CPPRESTSHARED_EXPORT ServerDB
{
public:
	const int EXPECTED_SCHEMA_VERSION = 1;

    ServerDB();
    ~ServerDB();

    void initDbIfEmpty();
    void reinitializeDb();
	void updateSchemaVersion(int version);
	int getSchemaVersion();

	bool addSession(const QString string_id, const int user_id, const QString user_login, const QString user_name, const QString random_secret, const QDateTime login_time, const bool is_for_db_only);
    bool addSession(const Session new_session);
    bool addSessions(const QList<Session> all_sessions);
    bool removeSession(const QString& string_id);
    bool wipeSessions();
    bool removeSessionsOlderThan(qint64 seconds);
    Session getSession(const QString& string_id);
    QList<Session> getAllSessions();
    int getSessionsCount();

    bool addUrl(const QString string_id, const QString filename, const QString path, const QString filename_with_path, const QString file_id, const qint64 size, const bool file_exists, const QDateTime created);
    bool addUrl(const UrlEntity new_url);
    bool addUrls(const QList<UrlEntity> all_urls);
    bool removeUrl(const QString& string_id);
    bool wipeUrls();
    bool removeUrlsOlderThan(qint64 seconds);
    bool isFileInStoreAlready(const QString& filename_with_path);
    UrlEntity getUrl(const QString& string_id);
    QList<UrlEntity> getAllUrls();
    int getUrlsCount();

    bool addFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing, QString json_content, const QDateTime requested);
    bool removeFileLocation(const QString& string_id);
    bool wipeFileLocations();
    bool removeFileLocationsOlderThan(qint64 seconds);
    QJsonDocument getFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing);
    bool hasFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing);
    void updateFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing);
    int getFileLocationsCount();

protected:
    QSharedPointer<QSqlDatabase> db_;

};




#endif // SERVERDB_H
