#include "SessionAndUrlBackupWorker.h"
#include "ServerDB.h"
#include "Log.h"
#include "Exceptions.h"
#include "Settings.h"

SessionAndUrlBackupWorker::SessionAndUrlBackupWorker(QList<Session> all_sessions, QList<UrlEntity> all_urls)
    : QRunnable()
    , all_sessions_(all_sessions)
    , all_urls_(all_urls)
{
}

void SessionAndUrlBackupWorker::run()
{
    try
    {
        ServerDB db = ServerDB();
        Log::info("Remove current backups");
        db.wipeSessions();
        db.wipeUrls();
        Log::info("Backup user sessions");
        db.addSessions(all_sessions_);
        Log::info("Backup temporary URLs");
        db.addUrls(all_urls_);

        Log::info("Removing old cache entries");
        int file_location_cache_lifespan = 0;
        try
        {
            file_location_cache_lifespan = Settings::integer("file_location_cache_lifespan");
        }
        catch (ProgrammingException& e)
        {
            file_location_cache_lifespan = 5*60; // lifespan of the cached FileLocation objects (default value)
            Log::warn(e.message() + " Use the default value: " + QString::number(file_location_cache_lifespan));
        }

        qint64 maximum_allowed_age = QDateTime::currentDateTime().toSecsSinceEpoch() - file_location_cache_lifespan;

        if (db.removeFileLocationsOlderThan(maximum_allowed_age))
        {
            Log::info("Old cache removed");
        }
        else
        {
            Log::error("Could not cleanup the cache");
        }
    }
    catch (DatabaseException& e)
    {
        Log::error("A database error has been detected while backing up sessions and URLs: " + e.message());
    }
    catch (Exception& e)
    {
        Log::error("An error has been detected while backing up sessions and URLs: " + e.message());
    }
}
