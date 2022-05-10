#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "BedFile.h"
#include "GeneSet.h"
#include "FileLocation.h"

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
	//Returns the processing system amplicon region file.
	virtual BedFile processingSystemAmplicons(int sys_id, bool ignore_if_missing) const = 0;
	//Returns the processing system genes.
	virtual GeneSet processingSystemGenes(int sys_id, bool ignore_if_missing) const = 0;
	//Returns secondary analysis locations for a sample.
	virtual QStringList secondaryAnalyses(QString processed_sample_name, QString analysis_type) const = 0;

	//Returns a FileLocation for a given file type of a processed sample
	virtual FileLocation processedSamplePath(const QString& processed_sample_id, PathType type) const = 0;
	//Returns a location for a GSvar file based on the corresponding job id
	virtual FileLocation analysisJobGSvarFile(const int& job_id) const = 0;
};

#endif // DATABASESERVICE_H
