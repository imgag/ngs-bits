#ifndef URLBACKUPWORKER_H
#define URLBACKUPWORKER_H

#include "cppREST_global.h"
#include <QRunnable>
#include "Log.h"
#include "Exceptions.h"
#include "UrlManager.h"

class CPPRESTSHARED_EXPORT UrlBackupWorker
    : public QRunnable
{

public:
    explicit UrlBackupWorker(QMap<QString, UrlEntity> current_storage);
    void run() override;

private:
    QMap<QString, UrlEntity> current_storage_;

};

#endif // URLBACKUPWORKER_H
