#include "TestFramework.h"

TEST_CLASS(VariantFilterRegions_Test)
{
Q_OBJECT
private slots:
	
	void byBED_TSV()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out1.tsv");
		COMPARE_FILES("out/VariantFilterRegions_out1.tsv", TESTDATA("data_out/VariantFilterRegions_out1.tsv"));
	}

	void byBED_VCF()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in2.vcf") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out2.vcf");
		COMPARE_FILES("out/VariantFilterRegions_out2.vcf", TESTDATA("data_out/VariantFilterRegions_out2.vcf"));
	}

	void byBED_TSV_inverted()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -invert -out out/VariantFilterRegions_out3.tsv");
		COMPARE_FILES("out/VariantFilterRegions_out3.tsv", TESTDATA("data_out/VariantFilterRegions_out3.tsv"));
	}

	void byString_TSV()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -r chr2:70,000,000-120,000,000 -out out/VariantFilterRegions_out4.tsv");
		COMPARE_FILES("out/VariantFilterRegions_out4.tsv", TESTDATA("data_out/VariantFilterRegions_out4.tsv"));
	}

};


