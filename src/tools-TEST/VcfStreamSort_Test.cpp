#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfStreamSort_Test)
{
private:
	
	TEST_METHOD(base_test)
	{
		EXECUTE("VcfStreamSort", "-n 4 -in " + TESTDATA("data_in/VcfStreamSort_in1.vcf") + " -out out/VcfStreamSort_out1.vcf");
		COMPARE_FILES("out/VcfStreamSort_out1.vcf", TESTDATA("data_out/VcfStreamSort_out1.vcf"));
		VCF_IS_VALID("out/VcfStreamSort_out1.vcf")
	}


	TEST_METHOD(several_variants_at_same_position)
	{
		EXECUTE("VcfStreamSort", "-n 4 -in " + TESTDATA("data_in/VcfStreamSort_in2.vcf") + " -out out/VcfStreamSort_out2.vcf");
		COMPARE_FILES("out/VcfStreamSort_out2.vcf", TESTDATA("data_out/VcfStreamSort_out2.vcf"));
		VCF_IS_VALID_HG19("out/VcfStreamSort_out2.vcf")
	}
};

