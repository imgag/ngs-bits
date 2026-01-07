#include "TestFramework.h"

TEST_CLASS(VcfPruneSourceVariant_Test)
{
private:

	TEST_METHOD(default_mode)
	{
		EXECUTE("VcfPruneSourceVariant", "-in " + TESTDATA("data_in/VcfPruneSourceVariant_in1.vcf") + " -out out/VcfPruneSourceVariant_out1.vcf");
		COMPARE_FILES("out/VcfPruneSourceVariant_out1.vcf", TESTDATA("data_out/VcfPruneSourceVariant_out1.vcf"));
	}

};
