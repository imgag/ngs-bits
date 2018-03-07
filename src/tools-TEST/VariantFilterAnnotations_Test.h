#include "TestFramework.h"

TEST_CLASS(VariantFilterAnnotations_Test)
{
Q_OBJECT
private slots:
	
	void no_filter()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -out out/VariantFilterAnnotations_out1.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out1.tsv", TESTDATA("data_out/VariantFilterAnnotations_out1.tsv"));
	}

	void maxAf()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -max_af 0.01 -out out/VariantFilterAnnotations_out2.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out2.tsv", TESTDATA("data_out/VariantFilterAnnotations_out2.tsv"));
	}

	void impact_ihdb()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -impact HIGH,MODERATE -max_ihdb 20 -out out/VariantFilterAnnotations_out3.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out3.tsv", TESTDATA("data_out/VariantFilterAnnotations_out3.tsv"));
	}

	void class_filters_genoAffected()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -min_class 3 -filters low_DP,low_MQM,low_QUAL -geno_affected comphet+hom -out out/VariantFilterAnnotations_out4.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out4.tsv", TESTDATA("data_out/VariantFilterAnnotations_out4.tsv"));
	}

	void maxSubAf()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -max_af_sub 0.01 -out out/VariantFilterAnnotations_out5.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out5.tsv", TESTDATA("data_out/VariantFilterAnnotations_out5.tsv"));
	}

	void multisample_maxAf_dominant()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -max_af 0.01 -geno_affected het -geno_control wt -out out/VariantFilterAnnotations_out6.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out6.tsv", TESTDATA("data_out/VariantFilterAnnotations_out6.tsv"));
	}

	void multisample_maxAf_recessive()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -max_af 0.01 -geno_affected comphet+hom -geno_control not_hom -out out/VariantFilterAnnotations_out7.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out7.tsv", TESTDATA("data_out/VariantFilterAnnotations_out7.tsv"));
	}

	void multisample_notWT()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -geno_affected not_wt -geno_control not_wt -out out/VariantFilterAnnotations_out8.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out8.tsv", TESTDATA("data_out/VariantFilterAnnotations_out8.tsv"));
	}

};


