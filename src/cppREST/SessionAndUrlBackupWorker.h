#ifndef SESSIONANDURLBACKUPWORKER_H
#define SESSIONANDURLBACKUPWORKER_H

#include "cppREST_global.h"
#include <QRunnable>
#include "Log.h"
#include "Exceptions.h"
#include "SessionManager.h"

class CPPRESTSHARED_EXPORT SessionAndUrlBackupWorker
    : public QRunnable
{

public:
    explicit  SessionAndUrlBackupWorker(QList<Session> all_sessions, QList<UrlEntity> all_urls);
    void run() override;

private:
    QList<Session> all_sessions_;
    QList<UrlEntity> all_urls_;

};

#endif // SESSIONANDURLBACKUPWORKER_H
