#include "TestFramework.h"

TEST_CLASS(VariantFilterRegions_Test)
{
Q_OBJECT
private slots:
	
	void filter_TSV()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out1.tsv");
		COMPARE_FILES("out/VariantFilterRegions_out1.tsv", TESTDATA("data_out/VariantFilterRegions_out1.tsv"));
	}

	void filter_VCF()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in2.vcf") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out2.vcf");
		COMPARE_FILES("out/VariantFilterRegions_out2.vcf", TESTDATA("data_out/VariantFilterRegions_out2.vcf"));
	}
	
};


