#include "SomaticCnvInterpreter.h"


bool SomaticCnvInterpreter::includeInReport(const int copy_number, const QByteArray& cnv_type, const SomaticGeneRole& gene_role)
{
	//Deletion + loss of function
	if(copy_number < 2 && gene_role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION) return true;

	//Homozygous deletion
	if(copy_number == 0) return true;

	//Amplification + activating
	if(copy_number > 2 && gene_role.role == SomaticGeneRole::Role::ACTIVATING) return true;

	//very strong amplicfications
	if(copy_number > 5 && gene_role.role != SomaticGeneRole::Role::LOSS_OF_FUNCTION) return true;

	//focal CNVs
	if(cnv_type.contains("focal")) return true;

	return false;
}
