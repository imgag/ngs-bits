#ifndef SOMATICVARIANTINTERPRETER_H
#define SOMATICVARIANTINTERPRETER_H

#include "cppNGS_global.h"
#include "VariantList.h"
#include <QVariant>
#include <QDateTime>

//struct with VICC (Variant Interpretation for Cancer Consortium) input configuration for SomaticVariantInterpreter
struct CPPNGSSHARED_EXPORT SomaticViccData
{
	enum class State
	{
		NOT_APPLICABLE,
		VICC_FALSE,
		VICC_TRUE
	};
	//very strong evidence
	State null_mutation_in_tsg = State::NOT_APPLICABLE; //null mutation, i.e. nonsense, framshifft, splice sites, initiation codon, exon deletion in TSG

	//strong evidence
	State known_oncogenic_aa = State::NOT_APPLICABLE; //same aa change is known as oncogenic
	State oncogenic_functional_studies = State::NOT_APPLICABLE; //functional studies support evidence
	State strong_cancerhotspot = State::NOT_APPLICABLE; // at least 50 samples with more than 10 affected

	//moderate evidence
	State located_in_canerhotspot = State::NOT_APPLICABLE; //Located in cancerhotspot
	State absent_from_controls = State::NOT_APPLICABLE; //not described in population databases
	State protein_length_change = State::NOT_APPLICABLE; //protein length changed, tresult of in-frame del/ins in oncogenes/TSG or stop loss in TSG
	State other_aa_known_oncogenic = State::NOT_APPLICABLE; //other AA change with same position is known to be oncogenic
	State weak_cancerhotspot = State::NOT_APPLICABLE;//less than 50 samples but particular aa change at least 10

	//supporting evidence
	State computational_evidence = State::NOT_APPLICABLE; //multiple lines of computational evidence, e.g. FATHMM and CADD
	State mutation_in_gene_with_etiology = State::NOT_APPLICABLE; //mutation is in gene with a malignancy with a single genetic etiology
	State very_weak_cancerhotspot = State::NOT_APPLICABLE; //mutation is listed as hotspot, but particular aa change below 10

	//benign very strong evidence
	State very_high_maf = State::NOT_APPLICABLE; //minor allele frequency is > 5%

	//benign strong evidence
	State benign_functional_studies = State::NOT_APPLICABLE; //well established functional studies show no oncogenic effect
	State high_maf = State::NOT_APPLICABLE; //minor allele frequency is > 1%

	//benign supporting evidence
	State benign_computational_evidence = State::NOT_APPLICABLE; //multiple lines of computational evidence suggest no oncogenic impact
	State synonymous_mutation = State::NOT_APPLICABLE; //synonymous_mutation mutation for which splicing prediction predict no impact to the splice consensus sequence or a creation of a new splice site

	QString comment = "";

	QString created_by;
	QDateTime created_at;

	QString last_updated_by;
	QDateTime last_updated_at;


		QString state2string(State s)
		{
			if(s==State::VICC_FALSE) return "FALSE";
			else if(s==State::VICC_TRUE) return "TRUE";
			else return "NOT_APPLICABLE";
		}

		QMap<QString,QString> configAsMap()
		{
			QMap<QString, QString> result;
			result["null_mutation_in_tsg"] = state2string(null_mutation_in_tsg);
			result["known_oncogenic_aa"] = state2string(known_oncogenic_aa);
			result["oncogenic_functional_studies"] = state2string(oncogenic_functional_studies);
			result["strong_cancerhotspot"] = state2string(strong_cancerhotspot);
			result["located_in_canerhotspot"] = state2string(located_in_canerhotspot);
			result["absent_from_controls"] = state2string(absent_from_controls);
			result["protein_length_change"] = state2string(protein_length_change);
			result["other_aa_known_oncogenic"] = state2string(other_aa_known_oncogenic);
			result["weak_cancerhotspot"] = state2string(weak_cancerhotspot);
			result["computational_evidence"] = state2string(computational_evidence);
			result["mutation_in_gene_with_etiology"] = state2string(mutation_in_gene_with_etiology);
			result["very_weak_cancerhotspot"] = state2string(very_weak_cancerhotspot);
			result["very_high_maf"] = state2string(very_high_maf);
			result["benign_functional_studies"] = state2string(benign_functional_studies);
			result["high_maf"] = state2string(high_maf);
			result["benign_computational_evidence"] = state2string(benign_computational_evidence);
			result["synonymous_mutation"] = state2string(synonymous_mutation);
			result["comment"] = comment;
			result["created_by"] = created_by;
			result["created_at"] = created_at.toString("yyyy-MM-dd hh:mm:ss");
			result["last_updated_by"] = last_updated_by;
			result["last_updated_at"] = last_updated_at.toString("yyyy-MM-dd hh:mm:ss");

			return result;
		}


	///returns whether input configuration is valid
	bool isValid() const
	{
		if(known_oncogenic_aa != State::NOT_APPLICABLE && located_in_canerhotspot != State::NOT_APPLICABLE) return false;
		if(strong_cancerhotspot != State::NOT_APPLICABLE && located_in_canerhotspot != State::NOT_APPLICABLE) return false;

		if(known_oncogenic_aa != State::NOT_APPLICABLE && other_aa_known_oncogenic != State::NOT_APPLICABLE) return false;
		if(strong_cancerhotspot != State::NOT_APPLICABLE && other_aa_known_oncogenic != State::NOT_APPLICABLE) return false;
		if(located_in_canerhotspot != State::NOT_APPLICABLE && other_aa_known_oncogenic != State::NOT_APPLICABLE) return false;

		if(located_in_canerhotspot != State::NOT_APPLICABLE && weak_cancerhotspot != State::NOT_APPLICABLE) return false;
		if(other_aa_known_oncogenic != State::NOT_APPLICABLE && weak_cancerhotspot != State::NOT_APPLICABLE) return false;


		return true;
	}

	///returns number of strong oncogenic evidence rules that apply positively
	int strongEvidenceCount() const
	{
		int count = 0;
		if(known_oncogenic_aa == State::VICC_TRUE) ++count;
		if(oncogenic_functional_studies == State::VICC_TRUE) ++count;
		if(strong_cancerhotspot == State::VICC_TRUE) ++count;
		return count;
	}

	///returns number of moderate oncogenic evidence rules that apply positively
	int moderateEvidenceCount() const
	{
		int count = 0;
		if(located_in_canerhotspot == State::VICC_TRUE) ++count;
		if(absent_from_controls == State::VICC_TRUE) ++count;
		if(protein_length_change == State::VICC_TRUE) ++count;
		if(other_aa_known_oncogenic == State::VICC_TRUE) ++count;
		if(weak_cancerhotspot == State::VICC_TRUE) ++count;
		return count;
	}

	///returns number of supporting oncogenic evidence rules that apply positively
	int supportingEvidenceCount() const
	{
		int count = 0;
		if(computational_evidence == State::VICC_TRUE) ++count;
		if(mutation_in_gene_with_etiology == State::VICC_TRUE) ++count;
		if(very_weak_cancerhotspot == State::VICC_TRUE) ++count;
		return count;
	}

	///returns number of strong benign evidence rules that apply positively
	int benignStrongEvidenceCount() const
	{
		int count = 0;
		if(benign_functional_studies == State::VICC_TRUE) ++count;
		if(high_maf == State::VICC_TRUE) ++count;
		return count;
	}

	///returns number of supporting benign evidence rules that apply positively
	int benignSupportingEvidenceCount() const
	{
		int count = 0;
		if(benign_computational_evidence == State::VICC_TRUE) ++count;
		if(synonymous_mutation == State::VICC_TRUE) ++count;
		return count;
	}
};


class CPPNGSSHARED_EXPORT SomaticVariantInterpreter
{
public:
	//result options of Vicc classification
	enum class Result
	{
		ONCOGENIC,
		LIKELY_ONCOGENIC,
		BENIGN,
		LIKELY_BENIGN,
		UNCERTAIN_SIGNIFICANCE
	};

	///Returns variant score according VICC rules; combines oncogenic and benign algorithm
	static Result viccScore(const SomaticViccData& input);
	///Returns variant score according VICC rules as string
	static QString viccScoreAsString(const SomaticViccData& input);

	///predicts VICC decision parameters based on variant annotations
	static SomaticViccData predictViccValue(const VariantList& vl, const Variant& var);
	///checks if all annotations are available for parameter prediction
	static bool checkAnnoForPrediction(const VariantList& vl);

	static Result resultFromString(QByteArray in);

private:
	///Returns result for oncogenicity score only
	static Result viccOncogenicRule(const SomaticViccData& input);
	///Returns result for benign score only
	static Result viccBenignRule(const SomaticViccData& input);


	SomaticVariantInterpreter() = delete;
};

#endif // SOMATICVARIANTINTERPRETER_H
