#include "SessionBackupWorker.h"
#include "Helper.h"

SessionBackupWorker::SessionBackupWorker(QMap<QString, Session> current_storage)
    : QRunnable()
    , current_storage_(current_storage)
{
}

void SessionBackupWorker::run()
{
    Log::info("Starting to backup session list");
    QSharedPointer<QFile> outfile = Helper::openFileForWriting(ServerHelper::getSessionBackupFileName(), false);

    QMapIterator<QString, Session> i(current_storage_);
    QTextStream out(outfile.data());

    while (i.hasNext())
    {
        i.next();
        if (i.value().user_id > 0)
        {
            out << i.key() << "\t" << i.value().user_id << "\t" << i.value().user_login << "\t" << i.value().user_name << "\t" << i.value().login_time.toSecsSinceEpoch() << "\t" << i.value().is_for_db_only << "\n";
        }
    }
    Log::info("Session backup is finished");
}
