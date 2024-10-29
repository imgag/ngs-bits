#ifndef IGVINITCACHEPHASETWOWORKER_H
#define IGVINITCACHEPHASETWOWORKER_H

#include <QRunnable>
#include "IGVInitCache.h"
#include "VariantList.h"

// This worker requests FileLocation information from the server using DatabaseService,
// it is started when a user opens a sample to reduce a delay for the initial IGV call
class IGVInitCachePhaseTwoWorker
    : public QObject
    , public QRunnable
{
    Q_OBJECT

public:
    IGVInitCachePhaseTwoWorker(const QString current_filename);
    void run();

private:
    QString current_filename_;
};

#endif // IGVINITCACHEPHASETWOWORKER_H
