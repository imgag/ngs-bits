#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "BedFile.h"
#include "GeneSet.h"

//Database access service interface.
class DatabaseService
{
public:
	//Returns if the database is enabled.
	virtual bool enabled() const = 0;

	//############################## processing system files ##############################
	//Returns the processing system target region file.
	virtual BedFile processingSystemRegions(int sys_id) const = 0;
	//Returns the processing system amplicon region file.
	virtual BedFile processingSystemAmplicons(int sys_id) const = 0;
	//Returns the processing system genes.
	virtual GeneSet processingSystemGenes(int sys_id) const = 0;
};

#endif // DATABASESERVICE_H
