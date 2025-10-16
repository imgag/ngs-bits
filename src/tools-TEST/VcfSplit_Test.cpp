#include "TestFramework.h"

TEST_CLASS(VcfSplit_Test)
{
private:
	
	TEST_METHOD(default_test)
	{
		EXECUTE_FAIL("VcfSplit", "-in " + TESTDATA("data_in/VcfSplit_in1.vcf") + " -lines 100 -out out/VcfSplit_out");
		COMPARE_FILES("out/VcfSplit_out0001.vcf", TESTDATA("data_out/VcfSplit_out0001.vcf"));
		COMPARE_FILES("out/VcfSplit_out0002.vcf", TESTDATA("data_out/VcfSplit_out0002.vcf"));
	}
};

