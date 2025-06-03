#include "ServerDB.h"
#include "ServerHelper.h"

ServerDB::ServerDB()
{
    QString db_host = Settings::string("gsvar_server_db_host");
    int db_port = Settings::integer("gsvar_server_db_port");
    QString db_name = Settings::string("gsvar_server_db_name");
    QString db_user = Settings::string("gsvar_server_db_user");
    QString db_pass = Settings::string("gsvar_server_db_pass");

    if (db_host.isEmpty() || db_port==0 || db_name.isEmpty() || db_user.isEmpty() || db_pass.isEmpty())
    {
        THROW(Exception, "Server database connection parameters have been set incorrectly. The server cannot operate like that");
    }

    db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", db_name + "_" + QUuid::createUuid().toString())));

    db_->setHostName(db_host);
    db_->setPort(db_port);
    db_->setDatabaseName(db_name);
    db_->setUserName(db_user);
    db_->setPassword(db_pass);

    if (!db_->open())
    {
        THROW(DatabaseException, "Could not connect to the server database: " + db_->lastError().text());
    }
}

ServerDB::~ServerDB()
{
    //close database and remove it
    QString connection_name = db_->connectionName();
    db_.clear();
    QSqlDatabase::removeDatabase(connection_name);
}

void ServerDB::initDbIfEmpty()
{
    Log::info("Creating new tables, if they do not exist");
    QString client_info_table = "CREATE TABLE IF NOT EXISTS client_info ("
                                "`id` INT(10) unsigned NOT NULL PRIMARY KEY AUTO_INCREMENT,"
                                "`version` VARCHAR(50),"
                                "`message` TEXT,"
                                "`date` BIGINT"
                                ");";

    QString user_notification = "CREATE TABLE IF NOT EXISTS user_notification ("
                                "`id` INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,"
                                "`string_id` VARCHAR(40),"
                                "`message` TEXT"
                                ");";

    QString sessions_table = "CREATE TABLE IF NOT EXISTS sessions ("
                             "`id` INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,"
                             "`string_id` VARCHAR(40) NOT NULL,"
                             "`user_id` INTEGER NOT NULL,"
                             "`user_login` TEXT NOT NULL,"
                             "`user_name` VARCHAR(40),"
                             "`login_time` BIGINT NOT NULL,"
                             "`is_for_db_only` INTEGER(1)"
                             ");";

    QString urls_table = "CREATE TABLE IF NOT EXISTS urls ("
                         "`id` INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,"
                         "`string_id` VARCHAR(40) NOT NULL,"
                         "`filename` TEXT NOT NULL,"
                         "`path` TEXT NOT NULL,"
                         "`filename_with_path` TEXT NOT NULL,"
                         "`file_id` TEXT NOT NULL,"
                         "`size` BIGINT NOT NULL,"
                         "`file_exists` INTEGER(1),"
                         "`created` BIGINT NOT NULL"
                         ");";

    QString file_locations_table = "CREATE TABLE IF NOT EXISTS file_locations ("
                             "`id` INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,"
                             "`filename_with_path` TEXT NOT NULL,"
                             "`type` VARCHAR(40) NOT NULL,"
                             "`locus` VARCHAR(40) NOT NULL,"
                             "`multiple_files` INTEGER(1),"
                             "`return_if_missing` INTEGER(1),"
                             "`json_content` TEXT NOT NULL,"
                             "`requested` BIGINT NOT NULL"
                             ");";

    QList<QString> filedb_tables = QList<QString>() << client_info_table << user_notification << sessions_table << urls_table << file_locations_table;
    for(int i = 0; i < filedb_tables.size(); i++)
    {
        QSqlQuery query(*(db_.data()));
        query.exec(filedb_tables[i]);
        bool success = query.lastError().text().trimmed().isEmpty();

        if(!success)
        {
            Log::error("Failed to create a table: " + query.lastError().text());
        }
    }
}

void ServerDB::reinitializeDb()
{
    Log::info("Erasing existing tables");
    QList<QString> table_name_list = QList<QString>() << "client_info" << "user_notification" << "sessions" << "urls" << "file_locations";
    for(int i = 0; i < table_name_list.size(); i++)
    {
        QSqlQuery query(*(db_.data()));
        query.exec("DROP TABLE IF EXISTS " + table_name_list[i]);
        bool success = query.lastError().text().trimmed().isEmpty();

        if(!success)
        {
            Log::error("Failed to remove a table: " + query.lastError().text());
        }
    }
    initDbIfEmpty();
}

bool ServerDB::addSession(const QString string_id, const int user_id, const QString user_login, const QString user_name, const QDateTime login_time, const bool is_for_db_only)
{
    qint64 login_time_as_num = login_time.toSecsSinceEpoch();
    QSqlQuery query(*(db_.data()));
    query.exec("INSERT INTO sessions (string_id, user_id, user_login, user_name, login_time, is_for_db_only)"
                                                       " VALUES (\""+string_id+"\", " + QString::number(user_id) + ", \"" + user_login + "\", \"" + user_name + "\", " + QString::number(login_time_as_num) + ", " + QString::number(is_for_db_only) + ")");
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to add a new session: " + query.lastError().text() + ", " + query.lastQuery());
    }

    return success;
}

bool ServerDB::addSession(const Session new_session)
{
    return addSession(new_session.string_id, new_session.user_id, new_session.user_login, new_session.user_name, new_session.login_time, new_session.is_for_db_only);
}

bool ServerDB::addSessions(const QList<Session> all_sessions)
{
    Log::info("Writing a new backup for sessions");
    int batch_size = 1000; //max value for multiple inserts in SQL

    int batch_count = (all_sessions.count() + batch_size - 1)/ batch_size;
    if ((all_sessions.count()%batch_size) == 0)
    {
        batch_count = all_sessions.count()/batch_size;
    }

    Log::info("Session batch count: " + QString::number(batch_count));
    int processed_items = 0;
    for (int i=0; i<batch_count; i++)
    {
        QString query_text = "INSERT INTO sessions (string_id, user_id, user_login, user_name, login_time, is_for_db_only) VALUES";
        for (int b=i*batch_size; b<((i+1)*batch_size); b++)
        {
            if (b>(all_sessions.count()-1)) break;

            qint64 login_time_as_num = all_sessions[b].login_time.toSecsSinceEpoch();
            query_text+="\n(\""+all_sessions[b].string_id+"\", " + QString::number(all_sessions[b].user_id) + ", \"" + all_sessions[b].user_login + "\", \"" + all_sessions[b].user_name + "\", " + QString::number(login_time_as_num) + ", " + QString::number(all_sessions[b].is_for_db_only) + "),";

            processed_items++;
        }
        query_text = query_text.left(query_text.size()-1);

        Log::info("Processed session count: " + QString::number(processed_items));
        QSqlQuery query(*(db_.data()));
        query.exec(query_text);
        bool success = query.lastError().text().trimmed().isEmpty();

        if(!success)
        {
            Log::error("Failed to add new sessions: " + query.lastError().text() + ", " + query.lastQuery());
            return false;
        }
    }

    return true;
}

bool ServerDB::removeSession(const QString& string_id)
{
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM sessions WHERE string_id = \"" + string_id + "\"");
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to remove the session '" + string_id + "': " + query.lastError().text());
    }
    return success;
}

bool ServerDB::wipeSessions()
{
    Log::info("Removing current backup for sessions");
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM sessions");
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to wipe the sessions: " + query.lastError().text());
    }
    return success;
}

bool ServerDB::removeSessionsOlderThan(qint64 seconds)
{
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM sessions WHERE login_time < " + QString::number(seconds));
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to remove sessions older than " + QDateTime::fromSecsSinceEpoch(seconds).toString("ddd MMMM d yy") +  ": " + query.lastError().text());
    }
    return success;
}

Session ServerDB::getSession(const QString& string_id)
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT * FROM sessions WHERE string_id = \"" + string_id + "\"");

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

    return Session();
}

QList<Session> ServerDB::getAllSessions()
{
    QList<Session> results;

    QSqlQuery query(*(db_.data()));
    query.exec("SELECT * FROM sessions");
    while (query.next())
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

    return results;
}

int ServerDB::getSessionsCount()
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT COUNT(*) FROM sessions");
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}

bool ServerDB::addUrl(const QString string_id, const QString filename, const QString path, const QString filename_with_path, const QString file_id, const qint64 size, const bool file_exists, const QDateTime created)
{
    qint64 created_as_num = created.toSecsSinceEpoch();
    QString query_text = "INSERT INTO urls (string_id, filename, path, filename_with_path, file_id, size, file_exists, created)"
                         " VALUES (\"" + string_id + "\", \"" + filename + "\", \"" + path + "\", \"" + filename_with_path + "\", \"" + file_id + "\", " +  QString::number(size) + ", " + QString::number(static_cast<int>(file_exists)) + ", " + QString::number(created_as_num) + ")";
    QSqlQuery query(*(db_.data()));
    query.exec(query_text);
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to add a new URL: " + query.lastError().text() + ", " + query.lastQuery());
    }

    return success;
}

bool ServerDB::addUrl(const UrlEntity new_url)
{
    return addUrl(new_url.string_id, new_url.filename, new_url.path, new_url.filename_with_path, new_url.file_id, new_url.size, new_url.file_exists, new_url.created);
}

bool ServerDB::addUrls(const QList<UrlEntity> all_urls)
{
    Log::info("Writing a new backup for URLs");
    int batch_size = 1000; //max value for multiple inserts in SQL

    int batch_count = (all_urls.count() + batch_size - 1)/ batch_size;
    if ((all_urls.count()%batch_size) == 0)
    {
        batch_count = all_urls.count()/batch_size;
    }

    Log::info("URL batch count: " + QString::number(batch_count));
    int processed_items = 0;
    for (int i=0; i<batch_count; i++)
    {
        QString query_text = "INSERT INTO urls (string_id, filename, path, filename_with_path, file_id, size, file_exists, created) VALUES";
        for (int b=i*batch_size; b<((i+1)*batch_size); b++)
        {
            if (b>(all_urls.count()-1)) break;

            qint64 created_as_num = all_urls[b].created.toSecsSinceEpoch();
            query_text+="\n(\"" + all_urls[b].string_id + "\", \"" + all_urls[b].filename + "\", \"" + all_urls[b].path + "\", \"" + all_urls[b].filename_with_path + "\", \"" + all_urls[b].file_id + "\", " + QString::number(all_urls[b].size) + ", " + QString::number(static_cast<int>(all_urls[b].file_exists)) + ", " + QString::number(created_as_num) + "),";

            processed_items++;
        }
        query_text = query_text.left(query_text.size()-1);

        Log::info("Processed URL count: " + QString::number(processed_items));
        QSqlQuery query(*(db_.data()));
        query.exec(query_text);
        bool success = query.lastError().text().trimmed().isEmpty();

        if(!success)
        {
            Log::error("Failed to add new URLs: " + query.lastError().text() + ", " + query.lastQuery());
            return false;
        }
    }
    return true;
}

bool ServerDB::removeUrl(const QString& string_id)
{
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM urls WHERE string_id = \"" + string_id + "\"");
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to remove the URL '" + string_id + "': " + query.lastError().text());
    }
    return success;
}

bool ServerDB::wipeUrls()
{
    Log::info("Removing current backup for URLs");
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM urls");
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to wipe the URLs : " + query.lastError().text());
    }
    return success;
}

bool ServerDB::removeUrlsOlderThan(qint64 seconds)
{
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM urls WHERE created < " + QString::number(seconds));
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to remove URLs older than " + QDateTime::fromSecsSinceEpoch(seconds).toString("ddd MMMM d yy") +  ": " + query.lastError().text());
    }
    return success;
}

bool ServerDB::isFileInStoreAlready(const QString& filename_with_path)
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT * FROM urls WHERE filename_with_path = \"" + filename_with_path + "\"");
    if (query.next())
    {
        return true;
    }
    return false;
}

UrlEntity ServerDB::getUrl(const QString& string_id)
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT * FROM urls WHERE string_id = \"" + string_id + "\"");

    if (query.next())
    {
        int index_string_id = query.record().indexOf("string_id");
        int index_filename = query.record().indexOf("filename");
        int index_path = query.record().indexOf("path");
        int index_filename_with_path = query.record().indexOf("filename_with_path");
        int index_file_id = query.record().indexOf("file_id");
        int index_size = query.record().indexOf("size");
        int index_file_exists = query.record().indexOf("file_exists");
        int index_created = query.record().indexOf("created");

        return UrlEntity(
            query.value(index_string_id).toString(),
            query.value(index_filename).toString(),
            query.value(index_path).toString(),
            query.value(index_filename_with_path).toString(),
            query.value(index_file_id).toString(),
            query.value(index_size).toLongLong(),
            query.value(index_file_exists).toBool(),
            QDateTime::fromSecsSinceEpoch(query.value(index_created).toLongLong())
            );
    }

    return UrlEntity();
}

QList<UrlEntity> ServerDB::getAllUrls()
{
    QList<UrlEntity> results;
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT * FROM urls");

    while (query.next())
    {
        int index_string_id = query.record().indexOf("string_id");
        int index_filename = query.record().indexOf("filename");
        int index_path = query.record().indexOf("path");
        int index_filename_with_path = query.record().indexOf("filename_with_path");
        int index_file_id = query.record().indexOf("file_id");
        int index_size = query.record().indexOf("size");
        int index_file_exists = query.record().indexOf("file_exists");
        int index_created = query.record().indexOf("created");

        results.append(
            UrlEntity(
                query.value(index_string_id).toString(),
                query.value(index_filename).toString(),
                query.value(index_path).toString(),
                query.value(index_filename_with_path).toString(),
                query.value(index_file_id).toString(),
                query.value(index_size).toLongLong(),
                query.value(index_file_exists).toBool(),
                QDateTime::fromSecsSinceEpoch(query.value(index_created).toLongLong())
                )
            );
    }

    return results;
}

int ServerDB::getUrlsCount()
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT COUNT(*) FROM urls");
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}

bool ServerDB::addFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing, QString json_content, const QDateTime requested)
{
    qint64 requested_as_num = requested.toSecsSinceEpoch();
    QString json_string = json_content.replace("'", "''");
    json_string = json_string.replace("\"", "\\\"");
    QSqlQuery query(*(db_.data()));
    query.exec("INSERT INTO file_locations (filename_with_path, type, locus, multiple_files, return_if_missing, json_content, requested)"
                                " VALUES (\"" + filename_with_path + "\", \"" + type + "\", \"" + locus + "\", " + QString::number(static_cast<int>(multiple_files)) + ", " + QString::number(static_cast<int>(return_if_missing)) + ", \"" + json_string + "\", " + QString::number(requested_as_num) + ")");
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to add a new FileLocation: " + query.lastError().text() + ", " + query.lastQuery());
    }

    return success;
}

bool ServerDB::removeFileLocation(const QString& id)
{
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM file_locations WHERE id = \"" + id + "\"");
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to remove the FileLocation '" + id + "': " + query.lastError().text());
    }
    return success;
}

bool ServerDB::wipeFileLocations()
{
    Log::info("Removing current backup for FileLocations");
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM file_locations");
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to wipe the FileLocations : " + query.lastError().text());
    }
    return success;
}

bool ServerDB::removeFileLocationsOlderThan(qint64 seconds)
{
    QSqlQuery query(*(db_.data()));
    query.exec("DELETE FROM file_locations WHERE requested < " + QString::number(seconds));
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to remove FileLocations older than " + QDateTime::fromSecsSinceEpoch(seconds).toString("ddd MMMM d yy") +  ": " + query.lastError().text());
    }
    return success;
}

QJsonDocument ServerDB::getFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing)
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT * FROM file_locations WHERE (filename_with_path = \"" + filename_with_path + "\" AND type = \"" + type + "\" AND locus = \"" + locus + "\" AND multiple_files=" + QString::number(static_cast<int>(multiple_files)) + " AND return_if_missing=" + QString::number(static_cast<int>(return_if_missing)) + ")");
    if (query.next())
    {
        int index_json_content = query.record().indexOf("json_content");      
        return QJsonDocument::fromJson(query.value(index_json_content).toString().toUtf8());
    }
    return QJsonDocument();
}

bool ServerDB::hasFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing)
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT * FROM file_locations WHERE (filename_with_path = \"" + filename_with_path + "\" AND type = \"" + type + "\" AND locus = \"" + locus + "\" AND multiple_files=" + QString::number(static_cast<int>(multiple_files)) + " AND return_if_missing=" + QString::number(static_cast<int>(return_if_missing)) + ")");
    if (query.next())
    {
        return true;
    }

    return false;
}

void ServerDB::updateFileLocation(const QString filename_with_path, const QString type, const QString locus, const bool multiple_files, const bool return_if_missing)
{
    QSqlQuery query(*(db_.data()));
    query.exec("UPDATE file_locations SET requested="+QString::number(QDateTime::currentDateTime().toSecsSinceEpoch())+" WHERE (filename_with_path = \"" + filename_with_path + "\" AND type = \"" + type + "\" AND locus = \"" + locus + "\" AND multiple_files=" + QString::number(static_cast<int>(multiple_files)) + " AND return_if_missing=" + QString::number(static_cast<int>(return_if_missing)) + ")");
}

int ServerDB::getFileLocationsCount()
{
    QSqlQuery query(*(db_.data()));
    query.exec("SELECT COUNT(*) FROM file_locations");
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}
