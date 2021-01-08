#include "SomaticVariantInterpreter.h"
#include "Exceptions.h"

SomaticVariantInterpreter::result SomaticVariantInterpreter::viccScore(const SomaticViccData &input)
{
	result oncogenic_result = viccOncogenicRule(input);
	result benign_result = viccBenignRule(input);

	if(oncogenic_result != UNCERTAIN_SIGNIFICANCE && benign_result != UNCERTAIN_SIGNIFICANCE) return UNCERTAIN_SIGNIFICANCE; //benign rule and oncogenic rule contradict each other
	if(oncogenic_result != UNCERTAIN_SIGNIFICANCE && benign_result == UNCERTAIN_SIGNIFICANCE) return oncogenic_result; //oncogenic rule applied, benign did not apply
	if(oncogenic_result == UNCERTAIN_SIGNIFICANCE && benign_result != UNCERTAIN_SIGNIFICANCE) return benign_result; //benign rule applied, oncogenic rule did not apply

	return UNCERTAIN_SIGNIFICANCE; //neither benign nor oncogenic rule applied
}

QString SomaticVariantInterpreter::viccScoreAsString(const SomaticViccData &input)
{
	result res = viccScore(input);
	if(res == ONCOGENIC) return "ONCOGENIC";
	if(res == LIKELY_ONCOGENIC) return "LIKELY_ONCOGENIC";
	if(res == BENIGN) return "BENIGN";
	if(res == LIKELY_BENIGN) return "LIKELY_BENIGN";

	return "UNCERTAIN_SIGNIFICANCE";
}

SomaticVariantInterpreter::result SomaticVariantInterpreter::viccOncogenicRule(const SomaticViccData &input)
{
	//Oncongenic
	if(input.null_mutation_in_tsg == SomaticViccData::TRUE123)
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
	if(input.null_mutation_in_tsg == SomaticViccData::TRUE123 && input.moderateEvidenceCount() >= 1) return LIKELY_ONCOGENIC;
	if(input.strongEvidenceCount() == 1 && input.moderateEvidenceCount() >= 1) return LIKELY_ONCOGENIC;
	if(input.strongEvidenceCount() == 1 && input.supportingEvidenceCount() >= 2) return LIKELY_ONCOGENIC;
	if(input.moderateEvidenceCount() >= 3) return LIKELY_ONCOGENIC;
	if(input.moderateEvidenceCount() >= 2 && input.supportingEvidenceCount() >= 2) return LIKELY_ONCOGENIC;
	if(input.moderateEvidenceCount() >= 1 && input.supportingEvidenceCount() >= 3) return LIKELY_ONCOGENIC;

	return UNCERTAIN_SIGNIFICANCE;
}

SomaticVariantInterpreter::result SomaticVariantInterpreter::viccBenignRule(const SomaticViccData &input)
{
	//Benign
	if(input.very_high_maf == SomaticViccData::TRUE123) return BENIGN;
	if(input.benignStrongEvidenceCount() >= 2) return BENIGN;

	//Likely Benign
	if(input.benignStrongEvidenceCount() >= 1 && input.benignSupportingEvidenceCount() >= 2) return LIKELY_BENIGN;

	return UNCERTAIN_SIGNIFICANCE;
}

SomaticViccData SomaticVariantInterpreter::predictViccValue(const VariantList &vl, const Variant &var)
{
	if(!checkAnnoForPrediction(vl))
	{
		THROW(ArgumentException, "Could not find all necessary annotation for VICC parameter prediction for variant " + var.toString());
	}

	SomaticViccData out;

	int i_cmc_mut_sign = vl.annotationIndexByName("CMC_mutation_significance");
	int i_ncg_tsg = vl.annotationIndexByName("ncg_tsg");
	int i_ncg_oncogene = vl.annotationIndexByName("ncg_oncogene");
	int i_gnomad = vl.annotationIndexByName("gnomAD");
	int i_clinvar = vl.annotationIndexByName("ClinVar");
	int i_cancerhotspots_total_count = vl.annotationIndexByName("CANCERHOTSPOTS_TOTAL_MUT");
	int i_cancerhotspots_alt_count = vl.annotationIndexByName("CANCERHOTSPOTS_ALT_COUNT");
	int i_gene_info = vl.annotationIndexByName("gene_info");
	int i_fathmm_mkl = vl.annotationIndexByName("fathmm-MKL");
	int i_cadd = vl.annotationIndexByName("CADD");


	bool is_tsg = var.annotations()[i_ncg_tsg].contains("1");
	bool is_oncogene = var.annotations()[i_ncg_oncogene].contains("1");


	int i_co_sp = vl.annotationIndexByName("coding_and_splicing");

	if(var.transcriptAnnotations(i_co_sp).count() == 0) return out;

	VariantTranscript trans = var.transcriptAnnotations(i_co_sp)[0]; //take first transcript

	//very strong oncogenic evidence: null mutation in TSG
	if(is_tsg && (trans.type.contains("stop_gained") || trans.type.contains("frameshift") || trans.type.contains("splice") || trans.type.contains("start_codon")) )
	{
		out.null_mutation_in_tsg = SomaticViccData::TRUE123;
	}
	else out.null_mutation_in_tsg = SomaticViccData::FALSE123;




	//strong oncogenic evidence 1: known oncogenic amino acid change;
	int clinvar_class = -1;
	if(var.annotations()[i_clinvar].contains("likely pathogenic")) clinvar_class = 4;
	else if(var.annotations()[i_clinvar].contains("pathogenic")) clinvar_class = 5;
	else if(var.annotations()[i_clinvar].contains("likely benign")) clinvar_class = 2;
	else if(var.annotations()[i_clinvar].contains("benign")) clinvar_class = 1;
	if(!var.annotations()[i_cmc_mut_sign].trimmed().isEmpty() && !var.annotations()[i_cmc_mut_sign].contains("Other") && clinvar_class >= 4) //CMC mutation significance is 4 or 5
	{
		out.known_oncogenic_aa = SomaticViccData::TRUE123;
	}
	else out.known_oncogenic_aa = SomaticViccData::FALSE123;


	//strong evidence 3: hot cancerhotspot
	int hotspots_total_count = var.annotations()[i_cancerhotspots_total_count].toInt();
	int hotspots_alt_count = var.annotations()[i_cancerhotspots_alt_count].toInt();
	if(hotspots_total_count >= 50 && hotspots_alt_count >= 10) out.strong_cancerhotspot = SomaticViccData::TRUE123;
	else out.strong_cancerhotspot = SomaticViccData::FALSE123;

	//moderate oncogenic 1: located in cancerhotspot; rule does not apply if known oncogenic or stron cancerhotspot applies.
	if(out.located_in_canerhotspot != SomaticViccData::NOT_APPLICABLE || out.strong_cancerhotspot != SomaticViccData::NOT_APPLICABLE) out.located_in_canerhotspot = SomaticViccData::NOT_APPLICABLE;
	//else to be discussed

	//moderate oncogenic 2: absent from control
	double gnomad_af = var.annotations()[i_gnomad].toDouble();
	if(gnomad_af < 0.001) out.absent_from_controls = SomaticViccData::TRUE123;
	else out.absent_from_controls = SomaticViccData::FALSE123;

	//moderate oncogenic 3: protein length changed
	if( (is_oncogene || is_tsg) && (trans.type.contains("inframe_insertion") || trans.type.contains("inframe_deletion")) )
	{
		out.protein_length_change = SomaticViccData::TRUE123;
	}
	else if(is_tsg && trans.type.contains("stop_lost")) out.protein_length_change = SomaticViccData::TRUE123;
	else out.protein_length_change = SomaticViccData::FALSE123;

	//moderate oncogenic 4: other aminoacid at same position is oncogenic
	if(out.known_oncogenic_aa != SomaticViccData::NOT_APPLICABLE || out.strong_cancerhotspot != SomaticViccData::NOT_APPLICABLE || out.located_in_canerhotspot != SomaticViccData::NOT_APPLICABLE)
	{
		out.other_aa_known_oncogenic = SomaticViccData::NOT_APPLICABLE;
	}

	//moderate oncogenic evidence 5: weak hotspot
	if(hotspots_total_count < 50 && hotspots_alt_count >= 10) out.weak_cancerhotspot = SomaticViccData::TRUE123;
	else out.weak_cancerhotspot = SomaticViccData::FALSE123;


	//supporting oncogenic evidence 1: computational evidence
	QByteArray tmp_fathmm =var.annotations()[i_fathmm_mkl];
	double fathmm = tmp_fathmm.append(",").split(',')[0].toDouble(); //take first value in list: refers to coding regions
	double cadd = var.annotations()[i_cadd].toDouble();
	if(fathmm > 0.9 && cadd > 20) out.computational_evidence = SomaticViccData::TRUE123;
	else out.computational_evidence = SomaticViccData::FALSE123;

	//supporting oncogenic evidence 2: TO BE DISKUSSED!

	//supporting oncogenic evidence 3: very weak cancerhotspot; located in weak ten-fold hotspot
	if(hotspots_alt_count > 0 && hotspots_alt_count < 10) out.very_weak_cancerhotspot = SomaticViccData::TRUE123;
	else out.very_weak_cancerhotspot = SomaticViccData::FALSE123;



	//very strong benign evidence: high minor AF
	if(gnomad_af > 0.05) out.very_high_maf = SomaticViccData::TRUE123;
	else out.very_high_maf = SomaticViccData::FALSE123;

	//strong benign evidence 1: appears in functional studies
	if(clinvar_class == 1 || clinvar_class == 2) out.benign_functional_studies = SomaticViccData::TRUE123;
	else out.benign_functional_studies = SomaticViccData::FALSE123;

	//strong benign evidence 2: high minor AF
	if(gnomad_af > 0.01) out.high_maf = SomaticViccData::TRUE123;
	else out.high_maf = SomaticViccData::FALSE123;

	//supporting benign evidence 1: TO BE DISCUSSED!


	//supporting benign evidence 2: synonymes and high o/e scores
	//take first entry of "gene_info":
	if(trans.type.contains("synonymous"))
	{
		QByteArray tmp_gene_info = var.annotations()[i_gene_info];
		QList<QByteArray> gene_info_parts = tmp_gene_info.append(',').split(',')[0].replace("(","").replace(")","").append(" ").split(' ');
		double oe_syn = std::numeric_limits<double>::quiet_NaN();
		for(QByteArray part : gene_info_parts)
		{
			if(!part.contains("oe_syn")) continue;
			oe_syn = part.trimmed().replace("oe_syn=","").toDouble();
		}
		if(BasicStatistics::isValidFloat(oe_syn) && oe_syn > 0.1) out.synonymous_mutation = SomaticViccData::TRUE123;
		else out.synonymous_mutation = SomaticViccData::FALSE123;
	}
	else out.synonymous_mutation = SomaticViccData::FALSE123;


	return out;
}

bool SomaticVariantInterpreter::checkAnnoForPrediction(const VariantList &vl)
{
	const QList<QString> annos = {"CMC_mutation_significance", "ncg_tsg", "ncg_oncogene", "gnomAD", "coding_and_splicing",  "ClinVar", "CANCERHOTSPOTS_TOTAL_MUT", "CANCERHOTSPOTS_ALT_COUNT", "coding_and_splicing", "gene_info", "fathmm-MKL", "CADD"};

	for(const auto& anno : annos)
	{
		if(vl.annotationIndexByName(anno, true, false) < 0) return false;
	}
	return true;
}
