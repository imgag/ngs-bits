#include "DatabaseServiceLocal.h"
#include "Settings.h"

DatabaseServiceLocal::DatabaseServiceLocal()
	: enabled_(Settings::boolean("NGSD_enabled"))
{
}

bool DatabaseServiceLocal::enabled() const
{
	return enabled_;
}

BedFile DatabaseServiceLocal::processingSystemRegions(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	return NGSD().processingSystemRegions(sys_id);
}

BedFile DatabaseServiceLocal::processingSystemAmplicons(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	return NGSD().processingSystemAmplicons(sys_id);
}

GeneSet DatabaseServiceLocal::processingSystemGenes(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	return NGSD().processingSystemGenes(sys_id);
}
