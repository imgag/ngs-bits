#include "TestFramework.h"

TEST_CLASS(SampleAncestry_Test)
{
private:

	TEST_METHOD(test_default)
	{
		EXECUTE("SampleAncestry", "-in " + TESTDATA("../cppNGS-TEST/data_in/ancestry_hg38.vcf.gz") + " -out out/SampleAncestry_out2.tsv");
		COMPARE_FILES("out/SampleAncestry_out2.tsv", TESTDATA("data_out/SampleAncestry_out2.tsv"));
	}
};

