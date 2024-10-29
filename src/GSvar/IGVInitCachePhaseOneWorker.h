#ifndef IGVINITCACHEPHASEONEWORKER_H
#define IGVINITCACHEWORKER_H

#include <QRunnable>
#include "IGVInitCache.h"
#include "VariantList.h"

// This worker requests FileLocation information from the server using FileLocationProvider,
// it is started when a user opens a sample to reduce a delay for the initial IGV call
class IGVInitCachePhaseOneWorker
    : public QObject
    , public QRunnable
{
    Q_OBJECT

public:
    IGVInitCachePhaseOneWorker(const AnalysisType analysis_type);
    void run();

private:
    AnalysisType analysis_type_;
};

#endif // IGVINITCACHEPHASEONEWORKER_H
