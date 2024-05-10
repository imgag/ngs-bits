#include "FileDbManager.h"
#include "Log.h"
#include <QVariant>

FileDbManager::FileDbManager(const QString path)
{
    file_database_ = QSqlDatabase::addDatabase("QSQLITE");
    file_database_.setDatabaseName(path);

    if (!file_database_.open())
    {
        Log::error("Database connection has failed");
    }
    else
    {
        Log::info("Database connection is ok");
    }
}

void FileDbManager::initDbIfEmpty()
{
    QString client_info_table = "CREATE TABLE IF NOT EXISTS client_info ("
        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
        "version TEXT,"
        "message TEXT,"
        "date REAL"
        ");";

    QString user_notification = "CREATE TABLE IF NOT EXISTS user_notification ("
        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
        "string_id TEXT,"
        "message TEXT"
        ");";

    QString sessions_table = "CREATE TABLE IF NOT EXISTS sessions ("
        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
        "string_id TEXT NOT NULL,"
        "user_id INTEGER NOT NULL,"
        "user_login TEXT NOT NULL,"
        "user_name TEXT,"
        "login_time REAL NOT NULL,"
        "is_for_db_only INTEGER"
        ");";

    QString urls_table = "CREATE TABLE IF NOT EXISTS urls ("
        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
        "string_id TEXT NOT NULL,"
        "filename TEXT NOT NULL,"
        "path TEXT NOT NULL,"
        "filename_with_path TEXT NOT NULL,"
        "file_id TEXT NOT NULL,"
        "created REAL NOT NULL"
        ");";

    QList<QString> filedb_tables = QList<QString>() << client_info_table << user_notification << sessions_table << urls_table;
    for(int i = 0; i < filedb_tables.size(); i++)
    {
        QSqlQuery query;
        query.prepare(filedb_tables[i]);
        bool success = query.exec();

        if(!success)
        {
            Log::error("Failed to create a table: " + query.lastError().text());
        }
    }
}

void FileDbManager::reinitializeDb()
{
    QList<QString> table_name_list = QList<QString>() << "client_info" << "user_notification" << "sessions" << "urls";
    for(int i = 0; i < table_name_list.size(); i++)
    {
        QSqlQuery query;
        query.prepare("DROP TABLE IF EXISTS " + table_name_list[i]);
        bool success = query.exec();

        if(!success)
        {
            Log::error(query.lastQuery());
            Log::error("Failed to remove a table: " + query.lastError().text());
        }
    }

    initDbIfEmpty();
}

bool FileDbManager::addSession(const QString string_id, const int user_id, const QString user_login, const QString user_name, const QDateTime login_time, const bool is_for_db_only)
{
    qint64 login_time_as_num = login_time.toSecsSinceEpoch();
    QSqlQuery query;
    query.prepare("INSERT INTO sessions (string_id, user_id, user_login, user_name, login_time, is_for_db_only)"
                  " VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(string_id);
    query.addBindValue(QString::number(user_id));
    query.addBindValue(user_login);
    query.addBindValue(user_name);
    query.addBindValue(QString::number(login_time_as_num));
    query.addBindValue(QString::number(is_for_db_only));

    bool success = query.exec();
    if(!success)
    {
        Log::error("Failed to add a new session: " + query.lastError().text() + ", " + query.lastQuery());
    }

    return success;
}

bool FileDbManager::addSession(const Session new_session)
{
    return addSession(new_session.string_id, new_session.user_id, new_session.user_login, new_session.user_name, new_session.login_time, new_session.is_for_db_only);
}

bool FileDbManager::removeSession(const QString& string_id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM sessions WHERE string_id = (:string_id)");
    query.bindValue(":string_id", string_id);
    bool success = query.exec();

    if(!success)
    {
        Log::error("Failed to remove the session '" + string_id + "': " + query.lastError().text());
    }
    return success;
}

Session FileDbManager::getSession(const QString& string_id)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM sessions WHERE string_id = (:string_id)");
    query.bindValue(":string_id", string_id);

    if (query.exec())
    {
        if (query.next())
        {
            int index_string_id = query.record().indexOf("string_id");
            int index_user_id = query.record().indexOf("user_id");
            int index_user_login = query.record().indexOf("user_login");
            int index_user_name = query.record().indexOf("user_name");
            int index_login_time = query.record().indexOf("login_time");
            int index_is_for_db_only = query.record().indexOf("is_for_db_only");

            return Session(
                query.value(index_string_id).toString(),
                query.value(index_user_id).toInt(),
                query.value(index_user_login).toString(),
                query.value(index_user_name).toString(),
                QDateTime::fromSecsSinceEpoch(query.value(index_login_time).toLongLong()),
                query.value(index_is_for_db_only).toInt()
            );
        }
    }
    return Session();
}

QList<Session> FileDbManager::getAllSessions()
{
    QList<Session> results;
    QSqlQuery query;
    query.prepare("SELECT * FROM sessions");

    if (query.exec())
    {
        if (query.next())
        {
            int index_string_id = query.record().indexOf("string_id");
            int index_user_id = query.record().indexOf("user_id");
            int index_user_login = query.record().indexOf("user_login");
            int index_user_name = query.record().indexOf("user_name");
            int index_login_time = query.record().indexOf("login_time");
            int index_is_for_db_only = query.record().indexOf("is_for_db_only");

            results.append(
                Session(
                    query.value(index_string_id).toString(),
                    query.value(index_user_id).toInt(),
                    query.value(index_user_login).toString(),
                    query.value(index_user_name).toString(),
                    QDateTime::fromSecsSinceEpoch(query.value(index_login_time).toLongLong()),
                    query.value(index_is_for_db_only).toInt()
                )
            );
        }
    }
    return results;
}

bool FileDbManager::addUrl(const QString string_id, const QString filename, const QString path, const QString filename_with_path, const QString file_id, const QDateTime created)
{
    qint64 created_as_num = created.toSecsSinceEpoch();
    QSqlQuery query;
    query.prepare("INSERT INTO urls (string_id, filename, path, filename_with_path, file_id, created)"
                  " VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(string_id);
    query.addBindValue(filename);
    query.addBindValue(path);
    query.addBindValue(filename_with_path);
    query.addBindValue(file_id);
    query.addBindValue(QString::number(created_as_num));

    bool success = query.exec();
    if(!success)
    {
        Log::error("Failed to add a new URL: " + query.lastError().text() + ", " + query.lastQuery());
    }

    return success;
}

bool FileDbManager::addUrl(const UrlEntity new_url)
{
    return addUrl(new_url.string_id, new_url.filename, new_url.path, new_url.filename_with_path, new_url.file_id, new_url.created);
}

bool FileDbManager::removeUrl(const QString& string_id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM urls WHERE string_id = (:string_id)");
    query.bindValue(":string_id", string_id);
    bool success = query.exec();

    if(!success)
    {
        Log::error("Failed to remove the URL '" + string_id + "': " + query.lastError().text());
    }
    return success;
}

bool FileDbManager::isFileInStoreAlrady(const QString& filename_with_path)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM urls WHERE filename_with_path = (:filename_with_path)");
    query.bindValue(":filename_with_path", filename_with_path);

    if (query.exec())
    {
        if (query.next())
        {
            return true;
        }
    }

    return false;
}

UrlEntity FileDbManager::getUrl(const QString& string_id)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM urls WHERE string_id = (:string_id)");
    query.bindValue(":string_id", string_id);

    if (query.exec())
    {
        if (query.next())
        {
            int index_string_id = query.record().indexOf("string_id");
            int index_filename = query.record().indexOf("filename");
            int index_path = query.record().indexOf("path");
            int index_filename_with_path = query.record().indexOf("filename_with_path");
            int index_file_id = query.record().indexOf("file_id");
            int index_created = query.record().indexOf("created");

            return UrlEntity(
                query.value(index_string_id).toString(),
                query.value(index_filename).toString(),
                query.value(index_path).toString(),
                query.value(index_filename_with_path).toString(),
                query.value(index_file_id).toString(),
                QDateTime::fromSecsSinceEpoch(query.value(index_created).toLongLong())
            );
        }
    }
    return UrlEntity();
}

QList<UrlEntity> FileDbManager::getAllUrls()
{
    QList<UrlEntity> results;
    QSqlQuery query;
    query.prepare("SELECT * FROM urls");

    if (query.exec())
    {
        if (query.next())
        {
            int index_string_id = query.record().indexOf("string_id");
            int index_filename = query.record().indexOf("filename");
            int index_path = query.record().indexOf("path");
            int index_filename_with_path = query.record().indexOf("filename_with_path");
            int index_file_id = query.record().indexOf("file_id");
            int index_created = query.record().indexOf("created");

            results.append(
                UrlEntity(
                    query.value(index_string_id).toString(),
                    query.value(index_filename).toString(),
                    query.value(index_path).toString(),
                    query.value(index_filename_with_path).toString(),
                    query.value(index_file_id).toString(),
                    QDateTime::fromSecsSinceEpoch(query.value(index_created).toLongLong())
                )
            );
        }
    }
    return results;
}
