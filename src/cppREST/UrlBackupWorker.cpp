#include "UrlBackupWorker.h"
#include "Helper.h"

UrlBackupWorker::UrlBackupWorker(QMap<QString, UrlEntity> current_storage)
    : QRunnable()
    , current_storage_(current_storage)
{
}

void UrlBackupWorker::run()
{
    Log::info("Starting to back up URL list");
    QSharedPointer<QFile> outfile = Helper::openFileForWriting(ServerHelper::getUrlStorageBackupFileName(), false);

    QMapIterator<QString, UrlEntity> i(current_storage_);
    QTextStream out(outfile.data());

    while (i.hasNext())
    {
        i.next();
        if (!i.value().filename.isEmpty())
        {
            out << i.key() << "\t" << i.value().filename << "\t" << i.value().path << "\t" << i.value().filename_with_path <<
                "\t" << i.value().file_id << "\t" << i.value().created.toSecsSinceEpoch() << "\n";
        }
    }
    Log::info("URL backup is finished");
}
