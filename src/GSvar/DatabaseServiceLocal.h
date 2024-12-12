#ifndef DATABASESERVICELOCAL_H
#define DATABASESERVICELOCAL_H

#include "DatabaseService.h"

class DatabaseServiceLocal
	: virtual public DatabaseService
{
public:
	DatabaseServiceLocal();
    virtual ~DatabaseServiceLocal() {}

    virtual bool enabled() const override;
	virtual QString checkPassword(const QString user_name, const QString password) const override;

	virtual BedFile processingSystemRegions(int sys_id, bool /*ignore_if_missing*/) const override;
	virtual GeneSet processingSystemGenes(int sys_id, bool /*ignore_if_missing*/) const override;
	virtual QStringList secondaryAnalyses(QString processed_sample_name, QString analysis_type) const override;

	virtual FileLocation processedSamplePath(const QString& processed_sample_id, PathType type) const override;
	virtual FileInfo analysisJobLatestLogInfo(const int& job_id) const override;
	virtual FileLocation analysisJobGSvarFile(const int& job_id) const override;	
	virtual FileLocation analysisJobLogFile(const int& job_id) const override;
	virtual QList<MultiSampleAnalysisInfo> getMultiSampleAnalysisInfo(QStringList& analyses) const override;

	virtual QStringList getRnaFusionPics(const QString& rna_id) const override;
	virtual QStringList getRnaExpressionPlots(const QString& rna_id) const override;

protected:
    //Throws an error if NGSD is not enabled
    void checkEnabled(QString function) const
    {
        if (!enabled_)
        {
            THROW(ProgrammingException, "NGSD is not enabled, but instance requested in '" + function + "'");
        }
    }
    bool enabled_;
};

#endif // DATABASESERVICE_H
