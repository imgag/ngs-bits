#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VariantFilterRegions_Test)
{
Q_OBJECT
private slots:
	
	void byBED_TSV()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out1.tsv -mode gsvar");
		COMPARE_FILES("out/VariantFilterRegions_out1.tsv", TESTDATA("data_out/VariantFilterRegions_out1.tsv"));
	}

	void byBED_VCF()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in2.vcf") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out2.vcf");
		COMPARE_FILES("out/VariantFilterRegions_out2.vcf", TESTDATA("data_out/VariantFilterRegions_out2.vcf"));
		VCF_IS_VALID("out/VariantFilterRegions_out2.vcf")
	}

	void byBED_TSV_invert()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out3.tsv -inv -mode gsvar");
		COMPARE_FILES("out/VariantFilterRegions_out3.tsv", TESTDATA("data_out/VariantFilterRegions_out3.tsv"));
	}

	void byString_TSV()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -r chr2:70,000,000-120,000,000 -out out/VariantFilterRegions_out4.tsv -mode gsvar");
		COMPARE_FILES("out/VariantFilterRegions_out4.tsv", TESTDATA("data_out/VariantFilterRegions_out4.tsv"));
	}

	void byBED_TSV_mark()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out5.tsv -mark off-target -mode gsvar");
		COMPARE_FILES("out/VariantFilterRegions_out5.tsv", TESTDATA("data_out/VariantFilterRegions_out5.tsv"));
	}

	void byBED_VCF_mark()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in2.vcf") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out6.vcf -mark off-target");
		COMPARE_FILES("out/VariantFilterRegions_out6.vcf", TESTDATA("data_out/VariantFilterRegions_out6.vcf"));
		VCF_IS_VALID("out/VariantFilterRegions_out6.vcf")
	}

	void byBED_TSV_mark_invert()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + TESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out7.tsv -mark off-target -inv -mode gsvar");
		COMPARE_FILES("out/VariantFilterRegions_out7.tsv", TESTDATA("data_out/VariantFilterRegions_out7.tsv"));
	}

	void byString_VCF_multisample()
	{
		EXECUTE("VariantFilterRegions", "-in " + TESTDATA("data_in/VariantFilterRegions_in3.vcf") + " -r chr1:156341000-156351795 -out out/VariantFilterRegions_out8.vcf");
		COMPARE_FILES("out/VariantFilterRegions_out8.vcf", TESTDATA("data_out/VariantFilterRegions_out8.vcf"));
		VCF_IS_VALID("out/VariantFilterRegions_out8.vcf")
	}
};


