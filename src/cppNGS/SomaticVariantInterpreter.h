#ifndef SOMATICVARIANTINTERPRETER_H
#define SOMATICVARIANTINTERPRETER_H

#include "cppNGS_global.h"
#include <QVariant>
#include <QDateTime>

//struct with input configuration for SomaticVariantInterpreter
struct CPPNGSSHARED_EXPORT SomaticViccData
{
	enum state
	{
		NOT_APPLICABLE,
		FALSE123,
		TRUE123
	};
	//very strong evidence
	state null_mutation_in_tsg = NOT_APPLICABLE; //null mutation, i.e. nonsense, framshifft, splice sites, initiation codon, exon deletion in TSG

	//strong evidence
	state known_oncogenic_aa = NOT_APPLICABLE; //same aa change is known as oncogenic
	state oncogenic_functional_studies = NOT_APPLICABLE; //functional studies support evidence
	state strong_cancerhotspot = NOT_APPLICABLE; // at least 50 samples with more than 10 affected

	//moderate evidence
	state located_in_canerhotspot = NOT_APPLICABLE; //Located in cancerhotspot
	state absent_from_controls = NOT_APPLICABLE; //not described in population databases
	state protein_length_change = NOT_APPLICABLE; //protein length changed, tresult of in-frame del/ins in oncogenes/TSG or stop loss in TSG
	state other_aa_known_oncogenic = NOT_APPLICABLE; //other AA change with same position is known to be oncogenic
	state weak_cancerhotspot = NOT_APPLICABLE;//less than 50 samples but particular aa change at least 10

	//supporting evidence
	state computational_evidence = NOT_APPLICABLE; //multiple lines of computational evidence, e.g. FATHMM and CADD
	state mutation_in_gene_with_etiology = NOT_APPLICABLE; //mutation is in gene with a malignancy with a single genetic etiology
	state very_weak_cancerhotspot = NOT_APPLICABLE; //mutation is listed as hotspot, but particular aa change below 10

	//benign very strong evidence
	state very_high_maf = NOT_APPLICABLE; //minor allele frequency is > 5%

	//benign strong evidence
	state benign_functional_studies = NOT_APPLICABLE; //well established functional studies show no oncogenic effect
	state high_maf = NOT_APPLICABLE; //minor allele frequency is > 1%

	//benign supporting evidence
	state benign_computational_evidence = NOT_APPLICABLE; //multiple lines of computational evidence suggest no oncogenic impact
	state synonymous_mutation = NOT_APPLICABLE; //synonymous_mutation mutation for which splicing prediction predict no impact to the splice consensus sequence or a creation of a new splice site

	QString comment = "";

	QString created_by;
	QDateTime created_at;

	QString last_updated_by;
	QDateTime last_updated_at;

	///returns whether input configuration is valid
	bool isValid() const
	{
		if(known_oncogenic_aa != NOT_APPLICABLE && located_in_canerhotspot != NOT_APPLICABLE) return false;
		if(strong_cancerhotspot != NOT_APPLICABLE && located_in_canerhotspot != NOT_APPLICABLE) return false;

		if(known_oncogenic_aa != NOT_APPLICABLE && other_aa_known_oncogenic != NOT_APPLICABLE) return false;
		if(strong_cancerhotspot != NOT_APPLICABLE && other_aa_known_oncogenic != NOT_APPLICABLE) return false;
		if(located_in_canerhotspot != NOT_APPLICABLE && other_aa_known_oncogenic != NOT_APPLICABLE) return false;

		if(located_in_canerhotspot != NOT_APPLICABLE && weak_cancerhotspot != NOT_APPLICABLE) return false;
		if(other_aa_known_oncogenic != NOT_APPLICABLE && weak_cancerhotspot != NOT_APPLICABLE) return false;


		return true;
	}

	///returns number of strong oncogenic evidence rules that apply positively
	int strongEvidenceCount() const
	{
		int count = 0;
		if(known_oncogenic_aa == TRUE123) ++count;
		if(oncogenic_functional_studies == TRUE123) ++count;
		if(strong_cancerhotspot == TRUE123) ++count;
		return count;
	}

	///returns number of moderate oncogenic evidence rules that apply positively
	int moderateEvidenceCount() const
	{
		int count = 0;
		if(located_in_canerhotspot == TRUE123) ++count;
		if(absent_from_controls == TRUE123) ++count;
		if(protein_length_change == TRUE123) ++count;
		if(other_aa_known_oncogenic == TRUE123) ++count;
		if(weak_cancerhotspot == TRUE123) ++count;
		return count;
	}

	///returns number of supporting oncogenic evidence rules that apply positively
	int supportingEvidenceCount() const
	{
		int count = 0;
		if(computational_evidence == TRUE123) ++count;
		if(mutation_in_gene_with_etiology == TRUE123) ++count;
		if(very_weak_cancerhotspot == TRUE123) ++count;
		return count;
	}

	///returns number of strong benign evidence rules that apply positively
	int benignStrongEvidenceCount() const
	{
		int count = 0;
		if(benign_functional_studies == TRUE123) ++count;
		if(high_maf == TRUE123) ++count;
		return count;
	}

	///returns number of supporting benign evidence rules that apply positively
	int benignSupportingEvidenceCount() const
	{
		int count = 0;
		if(benign_computational_evidence == TRUE123) ++count;
		if(synonymous_mutation == TRUE123) ++count;
		return count;
	}
};


class CPPNGSSHARED_EXPORT SomaticVariantInterpreter
{
public:
	//result options of Vicc classification
	enum result
	{
		ONCOGENIC,
		LIKELY_ONCOGENIC,
		BENIGN,
		LIKELY_BENIGN,
		UNCERTAIN_SIGNIFICANCE
	};

	///Returns variant score according VICC rules; combined oncogenic and benign algorithm
	static result viccScore(const SomaticViccData& input);


private:
	///Returns result for oncogenicity score only
	static result viccOncogenicRule(const SomaticViccData& input);
	///Returns result for benign score only
	static result viccBenignRule(const SomaticViccData& input);



private:
	SomaticVariantInterpreter() = delete;

};

#endif // SOMATICVARIANTINTERPRETER_H
