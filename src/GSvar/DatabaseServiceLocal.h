#ifndef DATABASESERVICELOCAL_H
#define DATABASESERVICELOCAL_H

#include "DatabaseService.h"
#include "NGSD.h"

class DatabaseServiceLocal
	: virtual public DatabaseService
{
public:
	DatabaseServiceLocal();
	virtual ~DatabaseServiceLocal() {}

	virtual bool enabled() const override;

	virtual BedFile processingSystemRegions(int sys_id) const override;
	virtual BedFile processingSystemAmplicons(int sys_id) const override;
	virtual GeneSet processingSystemGenes(int sys_id) const override;

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
