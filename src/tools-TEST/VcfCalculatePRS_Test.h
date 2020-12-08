#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfCalculatePRS_Test)
{
Q_OBJECT
private slots:

	void test_without_percentiles()
	{
		EXECUTE("VcfCalculatePRS", "-in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs " + TESTDATA("data_in/VcfCalculatePRS_prs1.vcf")
				+ " -out out/VcfCalculatePRS_out1.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out1.tsv", TESTDATA("data_out/VcfCalculatePRS_out1.tsv"));
	}

	void test_with_percentiles()
	{
		EXECUTE("VcfCalculatePRS", "-in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs " + TESTDATA("data_in/VcfCalculatePRS_prs2.vcf")
				+ " -out out/VcfCalculatePRS_out2.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out2.tsv", TESTDATA("data_out/VcfCalculatePRS_out2.tsv"));
	}

	void test_with_multiple_files()
	{
		EXECUTE("VcfCalculatePRS", "-in " + TESTDATA("data_in/VcfCalculatePRS_in1.vcf.gz") + " -prs " + TESTDATA("data_in/VcfCalculatePRS_prs1.vcf")
				+ " " + TESTDATA("data_in/VcfCalculatePRS_prs2.vcf") + " -out out/VcfCalculatePRS_out3.tsv");
		COMPARE_FILES("out/VcfCalculatePRS_out3.tsv", TESTDATA("data_out/VcfCalculatePRS_out3.tsv"));
	}

};
