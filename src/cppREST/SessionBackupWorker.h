#ifndef SESSIONBACKUPWORKER_H
#define SESSIONBACKUPWORKER_H

#include "cppREST_global.h"
#include <QRunnable>
#include "Log.h"
#include "Exceptions.h"
#include "SessionManager.h"

class CPPRESTSHARED_EXPORT SessionBackupWorker
    : public QRunnable
{

public:
    explicit SessionBackupWorker(QMap<QString, Session> current_storage);
    void run() override;

private:
    QMap<QString, Session> current_storage_;

};

#endif // SESSIONBACKUPWORKER_H
