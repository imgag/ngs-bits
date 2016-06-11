#include "TestFramework.h"

TEST_CLASS(VariantFilterAnnotations_Test)
{
Q_OBJECT
private slots:
	
	void no_filter()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.tsv") + " -out out/VariantFilterAnnotations_out1.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out1.tsv", TESTDATA("data_out/VariantFilterAnnotations_out1.tsv"));
	}

	void max_af()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.tsv") + " -max_af 0.01 -out out/VariantFilterAnnotations_out2.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out2.tsv", TESTDATA("data_out/VariantFilterAnnotations_out2.tsv"));
	}

	void impact_ihdb()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.tsv") + " -impact HIGH,MODERATE -max_ihdb 20 -out out/VariantFilterAnnotations_out3.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out3.tsv", TESTDATA("data_out/VariantFilterAnnotations_out3.tsv"));
	}

	void class_filters_comphet_genotype()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.tsv") + " -min_class 3 -comphet -filters low_DP,low_MQM,low_QUAL -genotype het -out out/VariantFilterAnnotations_out4.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out4.tsv", TESTDATA("data_out/VariantFilterAnnotations_out4.tsv"));
	}
};


