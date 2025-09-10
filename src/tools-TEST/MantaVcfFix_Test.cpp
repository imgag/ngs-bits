#include "TestFramework.h"


TEST_CLASS(MantaVcfFix_Test)
{
private:
	
	TEST_METHOD(manta_test)
	{
		EXECUTE("MantaVcfFix", "-in " + TESTDATA("data_in/MantaVcfFix_in1.vcf.gz") + " -out out/MantaVcfFix_out1.vcf");
		COMPARE_FILES("out/MantaVcfFix_out1.vcf", TESTDATA("data_out/MantaVcfFix_out1.vcf"));
	}
	
	TEST_METHOD(dragen_test)
	{
		EXECUTE("MantaVcfFix", "-in " + TESTDATA("data_in/MantaVcfFix_in2.vcf") + " -out out/MantaVcfFix_out2.vcf");
		COMPARE_FILES("out/MantaVcfFix_out2.vcf", TESTDATA("data_out/MantaVcfFix_out2.vcf"));
	}
};

