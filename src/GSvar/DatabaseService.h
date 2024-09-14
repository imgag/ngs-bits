#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "BedFile.h"
#include "GeneSet.h"
#include "FileLocation.h"
#include "FileInfo.h"
#include "NGSD.h"

//Database access service interface.
class DatabaseService
{
public:
	//Returns if the database is enabled.
	virtual bool enabled() const = 0;

	//Returns an error message, if the user name and/or password are/is incorrect.
	//An empty string is returned otherwise.
	virtual QString checkPassword(const QString user_name, const QString password) const = 0;

	//############################## processing system files ##############################
	//Returns the processing system target region file.
	virtual BedFile processingSystemRegions(int sys_id, bool ignore_if_missing) const = 0;
	//Returns the processing system genes.
	virtual GeneSet processingSystemGenes(int sys_id, bool ignore_if_missing) const = 0;
	//Returns secondary analysis locations for a sample.
	virtual QStringList secondaryAnalyses(QString processed_sample_name, QString analysis_type) const = 0;

	//Returns a FileLocation for a given file type of a processed sample
	virtual FileLocation processedSamplePath(const QString& processed_sample_id, PathType type) const = 0;
	//Return metdata for the logs of an analysis job by its id
	virtual FileInfo analysisJobLatestLogInfo(const int& job_id) const = 0;
	//Returns a location for a GSvar file based on the corresponding job id
	virtual FileLocation analysisJobGSvarFile(const int& job_id) const = 0;	
	//Returns a log file location object for a specific job based on its id
	virtual FileLocation analysisJobLogFile(const int& job_id) const = 0;
	//Returns a list of analysis names for multi-sample analyses
	virtual QList<MultiSampleAnalysisInfo> getMultiSampleAnalysisInfo(QStringList& analyses) const = 0;

	//Returns the list of fusion plots for RNA report
	virtual QStringList getRnaFusionPics(const QString& rna_id) const = 0;
	//Returns expression plots for RNA report
	virtual QStringList getRnaExpressionPlots(const QString& rna_id) const = 0;
};

#endif // DATABASESERVICE_H
