#ifndef SOMATICVARIANTINTERPRETER_TEST_H
#define SOMATICVARIANTINTERPRETER_TEST_H

#include "TestFramework.h"
#include "SomaticVariantInterpreter.h"

TEST_CLASS(SomaticVariantInterpreter_Test)
{
Q_OBJECT
private slots:

	void SomaticViccDataTest()
	{
		SomaticViccData input_data;
		IS_TRUE(input_data.isValid());

		input_data.known_oncogenic_aa = SomaticViccData::State::VICC_TRUE;

		input_data.located_in_canerhotspot = SomaticViccData::State::VICC_FALSE;
		IS_FALSE(input_data.isValid());
		S_EQUAL( input_data.configAsMap()["known_oncogenic_aa"], "TRUE" );
		S_EQUAL( input_data.configAsMap()["located_in_canerhotspot"], "FALSE" );

		input_data.located_in_canerhotspot = SomaticViccData::State::NOT_APPLICABLE;
		IS_TRUE(input_data.isValid());
		S_EQUAL( input_data.configAsMap()["located_in_canerhotspot"], "NOT_APPLICABLE" );

		input_data.other_aa_known_oncogenic = SomaticViccData::State::VICC_FALSE;
		IS_FALSE(input_data.isValid());
		S_EQUAL( input_data.configAsMap()["other_aa_known_oncogenic"], "FALSE" );


		//evidence counts
		input_data = SomaticViccData();
		I_EQUAL(input_data.strongEvidenceCount(), 0);
		input_data.known_oncogenic_aa = SomaticViccData::State::VICC_TRUE;
		input_data.strong_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(input_data.strongEvidenceCount(), 2);

		I_EQUAL(input_data.moderateEvidenceCount(), 0);
		input_data.absent_from_controls = SomaticViccData::State::VICC_TRUE;
		input_data.protein_length_change = SomaticViccData::State::VICC_TRUE;
		input_data.weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(input_data.moderateEvidenceCount(), 3);

		I_EQUAL(input_data.supportingEvidenceCount(), 0);
		input_data.computational_evidence = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(input_data.supportingEvidenceCount(), 1);
	}

	void viccRulesOncogenic()
	{
		SomaticViccData input_data;

		//Very strong evidence and 1x strong evidence
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//Very strong evidence and >=2 moderate
		input_data = SomaticViccData();
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.located_in_canerhotspot = SomaticViccData::State::VICC_TRUE;
		input_data.absent_from_controls = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//Very strong evidence and >=1 moderate and >=1 supporting
		input_data = SomaticViccData();
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.protein_length_change = SomaticViccData::State::VICC_TRUE;
		input_data.very_weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//Very strong evidence and and >=2 supporting
		input_data = SomaticViccData();
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.computational_evidence = SomaticViccData::State::VICC_TRUE;
		input_data.mutation_in_gene_with_etiology = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//>=2 strong evidence
		input_data = SomaticViccData();
		input_data.strong_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);
		input_data.known_oncogenic_aa = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//1x strong evidence and >= 3 moderate
		input_data = SomaticViccData();
		input_data.known_oncogenic_aa = SomaticViccData::State::VICC_TRUE;
		input_data.protein_length_change = SomaticViccData::State::VICC_TRUE;
		input_data.weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		input_data.located_in_canerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//1x strong and >=2xmoderate and >=2x supporting
		input_data = SomaticViccData();
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.absent_from_controls = SomaticViccData::State::VICC_TRUE;
		input_data.other_aa_known_oncogenic = SomaticViccData::State::VICC_TRUE;
		input_data.very_weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		input_data.mutation_in_gene_with_etiology = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//1x strong and >=1xmoderate and >=3x supporting
		input_data = SomaticViccData();
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.protein_length_change = SomaticViccData::State::VICC_TRUE;
		input_data.computational_evidence = SomaticViccData::State::VICC_TRUE;
		input_data.mutation_in_gene_with_etiology = SomaticViccData::State::VICC_TRUE;
		input_data.very_weak_cancerhotspot =  SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);
	}

	void viccRulesLikelyOncogenic()
	{
		SomaticViccData input_data;

		//1x very strong and >=1x moderate
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.other_aa_known_oncogenic = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC);

		//1x strong and >=1x moderate
		input_data = SomaticViccData();
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC);

		//1x strong and >=2x supporting
		input_data = SomaticViccData();
		input_data.known_oncogenic_aa = SomaticViccData::State::VICC_TRUE;
		input_data.mutation_in_gene_with_etiology = SomaticViccData::State::VICC_TRUE;
		input_data.very_weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC);

		//>=3x moderate
		input_data = SomaticViccData();
		input_data.other_aa_known_oncogenic = SomaticViccData::State::VICC_TRUE;
		input_data.absent_from_controls = SomaticViccData::State::VICC_TRUE;
		input_data.weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC);

		//>=2x moderate and >=2x supporting
		input_data = SomaticViccData();
		input_data.other_aa_known_oncogenic = SomaticViccData::State::VICC_TRUE;
		input_data.absent_from_controls = SomaticViccData::State::VICC_TRUE;
		input_data.computational_evidence = SomaticViccData::State::VICC_TRUE;
		input_data.mutation_in_gene_with_etiology = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC);

		//>=1x moderate and >=3x supportung
		input_data = SomaticViccData();
		input_data.located_in_canerhotspot = SomaticViccData::State::VICC_TRUE;
		input_data.computational_evidence = SomaticViccData::State::VICC_TRUE;
		input_data.mutation_in_gene_with_etiology = SomaticViccData::State::VICC_TRUE;
		input_data.very_weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC);
	}


	void viccRulesBenign()
	{
		SomaticViccData input_data;

		//1x very strong evidence
		input_data.very_high_maf = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::BENIGN);

		//2x strong evidence
		input_data = SomaticViccData();
		input_data.benign_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.high_maf = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::BENIGN);
	}

	void viccRulesLikelyBenign()
	{
		SomaticViccData input_data;

		//1x strong evidence and <=2x supporting evidence
		input_data.high_maf = SomaticViccData::State::VICC_TRUE;
		input_data.synonymous_mutation = SomaticViccData::State::VICC_TRUE;
		input_data.benign_computational_evidence = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_BENIGN);
	}

	void viccRulesUncertainSignificance()
	{
		SomaticViccData input_data;

		//1xvery strong oncogenic and nothing else = uncertain
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);

		//1x strong and 1x supporting oncogenic = uncertain
		input_data = SomaticViccData();
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.computational_evidence = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);

		//2x moderate oncogenic = uncertain
		input_data = SomaticViccData();
		input_data.absent_from_controls = SomaticViccData::State::VICC_TRUE;
		input_data.protein_length_change = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);

		//2x supporting benign = uncertain
		input_data = SomaticViccData();
		input_data.benign_computational_evidence = SomaticViccData::State::VICC_TRUE;
		input_data.synonymous_mutation = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);
	}

	//tests where options for benign and oncogeinc rules apply positively
	void viccScore()
	{
		SomaticViccData input_data;

		//oncogenic and not benign/not likely benign
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::ONCOGENIC);

		//likely oncogenic and not benign/not likely benign
		input_data = SomaticViccData();
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.weak_cancerhotspot = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC);

		//oncogenic and benign
		input_data = SomaticViccData();
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.very_high_maf = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);

		//oncogenic and likely benign
		input_data = SomaticViccData();
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.oncogenic_functional_studies = SomaticViccData::State::VICC_TRUE;
		input_data.high_maf = SomaticViccData::State::VICC_TRUE;
		input_data.synonymous_mutation = SomaticViccData::State::VICC_TRUE;
		input_data.benign_computational_evidence = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);

		//likely oncogenic and benign
		input_data = SomaticViccData();
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.other_aa_known_oncogenic = SomaticViccData::State::VICC_TRUE;
		input_data.very_high_maf = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);

		//likely oncogenic and likely benign
		input_data = SomaticViccData();
		input_data.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		input_data.other_aa_known_oncogenic = SomaticViccData::State::VICC_TRUE;
		input_data.high_maf = SomaticViccData::State::VICC_TRUE;
		input_data.synonymous_mutation = SomaticViccData::State::VICC_TRUE;
		input_data.benign_computational_evidence = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE);

		//benign and not oncogenic
		input_data = SomaticViccData();
		input_data.very_high_maf = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::BENIGN);

		//likely benign and not oncogneic
		input_data = SomaticViccData();
		input_data.high_maf = SomaticViccData::State::VICC_TRUE;
		input_data.synonymous_mutation = SomaticViccData::State::VICC_TRUE;
		input_data.benign_computational_evidence = SomaticViccData::State::VICC_TRUE;
		I_EQUAL(SomaticVariantInterpreter::viccScore(input_data), SomaticVariantInterpreter::Result::LIKELY_BENIGN);
	}

	void predictViccParameters()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/SomaticVariantInterpreter_predict.GSvar"));

		SomaticViccData predicted_params;

		//frameshift, but no TSG
		predicted_params = SomaticVariantInterpreter::predictViccValue(vl, vl[0]);
		I_EQUAL(predicted_params.null_mutation_in_tsg, SomaticViccData::State::VICC_FALSE);
		//variant is not strong cancerhotspot
		I_EQUAL(predicted_params.strong_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.absent_from_controls, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.protein_length_change, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.weak_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.computational_evidence, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.very_weak_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.very_high_maf, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.benign_functional_studies, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.high_maf, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.synonymous_mutation, SomaticViccData::State::VICC_FALSE);


		//TSG and frameshift
		predicted_params = SomaticVariantInterpreter::predictViccValue(vl, vl[1]);
		I_EQUAL(predicted_params.null_mutation_in_tsg, SomaticViccData::State::VICC_TRUE);

		//clinvar class 5 and CMC classification: os1 applies, strong cancerhotspot: os3 applies
		predicted_params = SomaticVariantInterpreter::predictViccValue(vl, vl[2]);
		I_EQUAL(predicted_params.known_oncogenic_aa, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.strong_cancerhotspot, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.absent_from_controls, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.computational_evidence, SomaticViccData::State::VICC_TRUE);



		//clinvar class 5 and no CMC classification: os1 does not apply, strong cancerhotspot: os3 applies
		predicted_params = SomaticVariantInterpreter::predictViccValue(vl, vl[3]);
		I_EQUAL(predicted_params.known_oncogenic_aa, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.strong_cancerhotspot, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.absent_from_controls, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.weak_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.computational_evidence, SomaticViccData::State::VICC_TRUE);

		I_EQUAL(predicted_params.very_high_maf, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(predicted_params.high_maf, SomaticViccData::State::VICC_TRUE);


		//benign variant
		predicted_params = SomaticVariantInterpreter::predictViccValue(vl, vl[4]);
		I_EQUAL(predicted_params.very_high_maf, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.benign_functional_studies, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.high_maf, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(predicted_params.synonymous_mutation, SomaticViccData::State::VICC_TRUE);
	}
};


#endif // SOMATICVARIANTINTERPRETER_TEST_H
