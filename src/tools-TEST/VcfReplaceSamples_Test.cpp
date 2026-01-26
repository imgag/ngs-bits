#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfReplaceSamples_Test)
{
private:

	TEST_METHOD(germline_single_sample)
	{
		EXECUTE("VcfReplaceSamples", "-in " + TESTDATA("data_in/VcfReplaceSamples_in1.vcf") + " -out out/VcfReplaceSamples_out1.vcf -ids NA12878_58=REP1");
		COMPARE_FILES("out/VcfReplaceSamples_out1.vcf", TESTDATA("data_out/VcfReplaceSamples_out1.vcf"));
		VCF_IS_VALID("out/VcfReplaceSamples_out1.vcf");
	}

	TEST_METHOD(tumor_normal_gzipped)
	{
		EXECUTE("VcfReplaceSamples", "-in " + TESTDATA("data_in/VcfReplaceSamples_in2.vcf.gz") + " -out out/VcfReplaceSamples_out2.vcf -ids NA12878x3_73=REP1,NA12877_49=REP2");
		COMPARE_FILES("out/VcfReplaceSamples_out2.vcf", TESTDATA("data_out/VcfReplaceSamples_out2.vcf"));
		VCF_IS_VALID("out/VcfReplaceSamples_out2.vcf");
	}
};
