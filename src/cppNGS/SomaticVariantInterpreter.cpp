#include "SomaticVariantInterpreter.h"

SomaticVariantInterpreter::result SomaticVariantInterpreter::viccScore(const SomaticVariantInterpreterInput &input)
{
	result oncogenic_result = viccOncogenicRule(input);
	result benign_result = viccBenignRule(input);

	if(oncogenic_result != UNCERTAIN_SIGNIFICANCE && benign_result != UNCERTAIN_SIGNIFICANCE) return UNCERTAIN_SIGNIFICANCE; //benign rule and oncogenic rule contradict each other
	if(oncogenic_result != UNCERTAIN_SIGNIFICANCE && benign_result == UNCERTAIN_SIGNIFICANCE) return oncogenic_result; //oncogenic rule applied, benign did not apply
	if(oncogenic_result == UNCERTAIN_SIGNIFICANCE && benign_result != UNCERTAIN_SIGNIFICANCE) return benign_result; //benign rule applied, oncogenic rule did not apply

	return UNCERTAIN_SIGNIFICANCE; //neither benign nor oncogenic rule applied
}

SomaticVariantInterpreter::result SomaticVariantInterpreter::viccOncogenicRule(const SomaticVariantInterpreterInput &input)
{
	//Oncongenic
	if(input.null_mutation_in_tsg == SomaticVariantInterpreterInput::TRUE)
	{
		if(input.strongEvidenceCount() >= 1) return ONCOGENIC;
		if(input.moderateEvidenceCount() >= 2) return ONCOGENIC;
		if(input.moderateEvidenceCount() >= 1 && input.supportingEvidenceCount() >= 1) return ONCOGENIC;
		if(input.supportingEvidenceCount() >= 2) return ONCOGENIC;
	}
	if(input.strongEvidenceCount() >=2) return ONCOGENIC;
	if(input.strongEvidenceCount() == 1)
	{
		if(input.moderateEvidenceCount() >= 3) return ONCOGENIC;
		if(input.moderateEvidenceCount() >=2 && input.supportingEvidenceCount() >= 2) return ONCOGENIC;
		if(input.moderateEvidenceCount() == 1 && input.supportingEvidenceCount() >= 3) return ONCOGENIC;
	}

	//Likely oncogenic
	if(input.null_mutation_in_tsg == SomaticVariantInterpreterInput::TRUE && input.moderateEvidenceCount() >= 1) return LIKELY_ONCOGENIC;
	if(input.strongEvidenceCount() == 1 && input.moderateEvidenceCount() >= 1) return LIKELY_ONCOGENIC;
	if(input.strongEvidenceCount() == 1 && input.supportingEvidenceCount() >= 2) return LIKELY_ONCOGENIC;
	if(input.moderateEvidenceCount() >= 3) return LIKELY_ONCOGENIC;
	if(input.moderateEvidenceCount() >= 2 && input.supportingEvidenceCount() >= 2) return LIKELY_ONCOGENIC;
	if(input.moderateEvidenceCount() >= 1 && input.supportingEvidenceCount() >= 3) return LIKELY_ONCOGENIC;

	return UNCERTAIN_SIGNIFICANCE;
}

SomaticVariantInterpreter::result SomaticVariantInterpreter::viccBenignRule(const SomaticVariantInterpreterInput &input)
{
	//Benign
	if(input.very_high_maf == SomaticVariantInterpreterInput::TRUE) return BENIGN;
	if(input.benignStrongEvidenceCount() >= 2) return BENIGN;

	//Likely Benign
	if(input.benignStrongEvidenceCount() >= 1 && input.benignSupportingEvidenceCount() >= 2) return LIKELY_BENIGN;

	return UNCERTAIN_SIGNIFICANCE;
}
