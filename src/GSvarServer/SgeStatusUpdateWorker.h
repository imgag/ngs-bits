#ifndef SGESTATUSUPDATEWORKER_H
#define SGESTATUSUPDATEWORKER_H

#include <QRunnable>
#include "Log.h"
#include "Exceptions.h"

class SgeStatusUpdateWorker
    : public QRunnable
{

public:
    explicit  SgeStatusUpdateWorker();
    void run() override;
};

#endif // SGESTATUSUPDATEWORKER_H
