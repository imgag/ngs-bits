#ifndef IGVINITCACHEWORKER_H
#define IGVINITCACHEWORKER_H

#include "Background/BackgroundWorkerBase.h"
#include "IgvSessionManager.h"
#include "VariantList.h"

// This worker requests FileLocation information from the server and
// is started only when a user opens a sample to reduce a delay for the initial IGV call
class IGVInitCacheWorker
    : public BackgroundWorkerBase
{
    Q_OBJECT

public:
    IGVInitCacheWorker(const AnalysisType analysis_type, const QString current_filename);
    void process() override;

private:
    AnalysisType analysis_type_;
    QString current_filename_;
};

#endif // IGVINITCACHEWORKER_H
