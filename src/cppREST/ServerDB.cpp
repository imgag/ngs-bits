#include "ServerDB.h"
#include "ServerHelper.h"

ServerDB::ServerDB()
{
    QString db_host = ServerHelper::getStringSettingsValue("gsvar_server_db_host");
    int db_port = ServerHelper::getNumSettingsValue("gsvar_server_db_port");
    QString db_name = ServerHelper::getStringSettingsValue("gsvar_server_db_name");
    QString db_user = ServerHelper::getStringSettingsValue("gsvar_server_db_user");
    QString db_pass = ServerHelper::getStringSettingsValue("gsvar_server_db_pass");

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
                         "`created` BIGINT NOT NULL"
                         ");";

    QList<QString> filedb_tables = QList<QString>() << client_info_table << user_notification << sessions_table << urls_table;
    for(int i = 0; i < filedb_tables.size(); i++)
    {
        QSqlQuery query = db_->exec(filedb_tables[i]);
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
    QList<QString> table_name_list = QList<QString>() << "client_info" << "user_notification" << "sessions" << "urls";
    for(int i = 0; i < table_name_list.size(); i++)
    {
        QSqlQuery query = db_->exec("DROP TABLE IF EXISTS " + table_name_list[i]);
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
    QSqlQuery query = db_->exec("INSERT INTO sessions (string_id, user_id, user_login, user_name, login_time, is_for_db_only)"
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

bool ServerDB::removeSession(const QString& string_id)
{
    QSqlQuery query = db_->exec("DELETE FROM sessions WHERE string_id = \"" + string_id + "\"");
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to remove the session '" + string_id + "': " + query.lastError().text());
    }
    return success;
}

bool ServerDB::removeSessionsOlderThan(qint64 seconds)
{
    QSqlQuery query = db_->exec("DELETE FROM sessions WHERE login_time < " + QString::number(seconds));
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to remove sessions older than " + QDateTime::fromSecsSinceEpoch(seconds).toString("ddd MMMM d yy") +  ": " + query.lastError().text());
    }
    return success;
}

Session ServerDB::getSession(const QString& string_id)
{
    QSqlQuery query = db_->exec("SELECT * FROM sessions WHERE string_id = \"" + string_id + "\"");

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

    QSqlQuery query = db_->exec("SELECT * FROM sessions");
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

    return results;
}

int ServerDB::getSessionsCount()
{
    QSqlQuery query = db_->exec("SELECT COUNT(*) FROM sessions");
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}

bool ServerDB::addUrl(const QString string_id, const QString filename, const QString path, const QString filename_with_path, const QString file_id, const QDateTime created)
{
    qint64 created_as_num = created.toSecsSinceEpoch();

    QSqlQuery query = db_->exec("INSERT INTO urls (string_id, filename, path, filename_with_path, file_id, created)"
                                                       " VALUES (\"" + string_id + "\", \"" + filename + "\", \"" + path + "\", \"" + filename_with_path + "\", \"" + file_id + "\", " + QString::number(created_as_num) + ")");
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to add a new URL: " + query.lastError().text() + ", " + query.lastQuery());
    }

    return success;
}

bool ServerDB::addUrl(const UrlEntity new_url)
{
    return addUrl(new_url.string_id, new_url.filename, new_url.path, new_url.filename_with_path, new_url.file_id, new_url.created);
}

bool ServerDB::removeUrl(const QString& string_id)
{
    QSqlQuery query = db_->exec("DELETE FROM urls WHERE string_id = \"" + string_id + "\"");
    bool success = query.lastError().text().trimmed().isEmpty();
    if(!success)
    {
        Log::error("Failed to remove the URL '" + string_id + "': " + query.lastError().text());
    }
    return success;
}

bool ServerDB::removeUrlsOlderThan(qint64 seconds)
{
    QSqlQuery query = db_->exec("DELETE FROM urls WHERE created < " + QString::number(seconds));
    bool success = query.lastError().text().trimmed().isEmpty();

    if(!success)
    {
        Log::error("Failed to remove URLs older than " + QDateTime::fromSecsSinceEpoch(seconds).toString("ddd MMMM d yy") +  ": " + query.lastError().text());
    }
    return success;
}

bool ServerDB::isFileInStoreAlready(const QString& filename_with_path)
{
    QSqlQuery cur_query = db_->exec("SELECT * FROM urls WHERE filename_with_path = \"" + filename_with_path + "\"");
    if (cur_query.next())
    {
        return true;
    }
    return false;
}

UrlEntity ServerDB::getUrl(const QString& string_id)
{
    QSqlQuery query = db_->exec("SELECT * FROM urls WHERE string_id = \"" + string_id + "\"");

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

    return UrlEntity();
}

QList<UrlEntity> ServerDB::getAllUrls()
{
    QList<UrlEntity> results;
    QSqlQuery query = db_->exec("SELECT * FROM urls");

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

    return results;
}

int ServerDB::getUrlsCount()
{
    QSqlQuery query = db_->exec("SELECT COUNT(*) FROM urls");
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}
