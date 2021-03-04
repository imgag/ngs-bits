#include "SomaticCnvInterpreter.h"


bool SomaticCnvInterpreter::includeInReport(const CnvList& cnv_list,const CopyNumberVariant &cnv, const SomaticGeneRole& gene_role)
{
	int cn = cnv.copyNumber(cnv_list.annotationHeaders());

	//Deletion + loss of function
	if(cn < 2 && gene_role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION) return true;

	//Homozygous deletion
	if(cn == 0) return true;

	//Amplification + activating
	if(cn > 2 && gene_role.role == SomaticGeneRole::Role::ACTIVATING) return true;

	//very strong amplicfications
	if(cn > 5) return true;

	//focal CNVs
	int i_cnv_type = cnv_list.annotationIndexByName("cnv_type", true);
	if(cnv.annotations()[i_cnv_type].contains("focal")) return true;

	return false;
}
