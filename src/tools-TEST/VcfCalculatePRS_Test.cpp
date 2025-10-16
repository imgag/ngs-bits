#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfCalculatePRS_Test)
{
private:

	TEST_METHOD(test_without_percentiles)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs1.vcf") + " -out out/VcfCalculatePRS_out1.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out1.tsv", TESTDATA("data_out/VcfCalculatePRS_out1.tsv"));
	}

	TEST_METHOD(test_with_percentiles)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs2.vcf") + " -out out/VcfCalculatePRS_out2.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out2.tsv", TESTDATA("data_out/VcfCalculatePRS_out2.tsv"));
	}

	TEST_METHOD(test_with_multiple_files)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs1.vcf") + " " + TESTDATA("data_in/VcfCalculatePRS_prs2.vcf") + " -out out/VcfCalculatePRS_out3.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out3.tsv", TESTDATA("data_out/VcfCalculatePRS_out3.tsv"));
	}

	TEST_METHOD(test_with_details_tsv)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs2.vcf") + " -out out/VcfCalculatePRS_out4.tsv -details out/VcfCalculatePRS_out4_details.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out4.tsv", TESTDATA("data_out/VcfCalculatePRS_out2.tsv"));
		COMPARE_FILES("out/VcfCalculatePRS_out4_details.tsv", TESTDATA("data_out/VcfCalculatePRS_out4_details.tsv"));
	}

	TEST_METHOD(test_with_reference_effect_allele) //flagged in INFO column
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs3.vcf") + " -out out/VcfCalculatePRS_out5.tsv -details out/VcfCalculatePRS_out5_details.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out5.tsv", TESTDATA("data_out/VcfCalculatePRS_out5.tsv"));
		COMPARE_FILES("out/VcfCalculatePRS_out5_details.tsv", TESTDATA("data_out/VcfCalculatePRS_out5_details.tsv"));
	}

	TEST_METHOD(test_with_reference_effect_allele2) // written as A>A
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs4.vcf") + " -out out/VcfCalculatePRS_out6.tsv -details out/VcfCalculatePRS_out6_details.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out6.tsv", TESTDATA("data_out/VcfCalculatePRS_out5.tsv"));
		COMPARE_FILES("out/VcfCalculatePRS_out6_details.tsv", TESTDATA("data_out/VcfCalculatePRS_out5_details.tsv"));
	}

	TEST_METHOD(test_with_reference_effect_allele3) // written as A>.
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs5.vcf") + " -out out/VcfCalculatePRS_out7.tsv -details out/VcfCalculatePRS_out7_details.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out7.tsv", TESTDATA("data_out/VcfCalculatePRS_out5.tsv"));
		COMPARE_FILES("out/VcfCalculatePRS_out7_details.tsv", TESTDATA("data_out/VcfCalculatePRS_out5_details.tsv"));
	}

	TEST_METHOD(test_with_imputed)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCalculatePRS", "-bam " + TESTDATA("data_in/VcfCalculatePRS_in1.bam") + " -in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs "
				+ TESTDATA("data_in/VcfCalculatePRS_prs6.vcf") + " -out out/VcfCalculatePRS_out8.tsv -details out/VcfCalculatePRS_out8_details.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out8.tsv", TESTDATA("data_out/VcfCalculatePRS_out6.tsv"));
		COMPARE_FILES("out/VcfCalculatePRS_out8_details.tsv", TESTDATA("data_out/VcfCalculatePRS_out6_details.tsv"));
	}

};
