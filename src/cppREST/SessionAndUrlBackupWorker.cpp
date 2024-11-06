#include "SessionAndUrlBackupWorker.h"

SessionAndUrlBackupWorker::SessionAndUrlBackupWorker(QList<Session> all_sessions, QList<UrlEntity> all_urls)
    : QRunnable()
    , all_sessions_(all_sessions)
    , all_urls_(all_urls)
{
}

void SessionAndUrlBackupWorker::run()
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
    qint64 five_minutes_ago = QDateTime::currentDateTime().toSecsSinceEpoch() - (5*60);

    if (db.removeFileLocationsOlderThan(five_minutes_ago))
    {
        Log::info("Old cache removed");
    }
    else
    {
        Log::error("Could not cleanup the cache");
    }
}
