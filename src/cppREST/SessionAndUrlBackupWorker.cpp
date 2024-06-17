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
}
