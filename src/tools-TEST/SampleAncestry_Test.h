#include "TestFramework.h"

TEST_CLASS(SampleAncestry_Test)
{
Q_OBJECT
private slots:
	
	void test_default()
	{
		EXECUTE("SampleAncestry", "-in " + TESTDATA("../cppNGS-TEST/data_in/ancestry.vcf.gz") + " -build hg19 -out out/SampleAncestry_out1.tsv");
		COMPARE_FILES("out/SampleAncestry_out1.tsv", TESTDATA("data_out/SampleAncestry_out1.tsv"));
	}

	void test_hg38()
	{
		EXECUTE("SampleAncestry", "-build hg38 -in " + TESTDATA("../cppNGS-TEST/data_in/ancestry_hg38.vcf.gz") + " -out out/SampleAncestry_out2.tsv");
		COMPARE_FILES("out/SampleAncestry_out2.tsv", TESTDATA("data_out/SampleAncestry_out2.tsv"));
	}
};

