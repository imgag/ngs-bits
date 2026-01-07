#include "TestFramework.h"

TEST_CLASS(VcfStoreSourceVariant_Test)
{
private:
	
	TEST_METHOD(test01)
	{
		EXECUTE("VcfStoreSourceVariant", "-in " + TESTDATA("data_in/VcfStoreSourceVariant_in1.vcf") + " -out out/VcfStoreSourceVariant_out1.vcf");
		COMPARE_FILES("out/VcfStoreSourceVariant_out1.vcf", TESTDATA("data_out/VcfStoreSourceVariant_out1.vcf"));
	}

	TEST_METHOD(test02)
	{
		EXECUTE("VcfStoreSourceVariant", "-in " + TESTDATA("data_in/VcfStoreSourceVariant_in2.vcf") + " -out out/VcfStoreSourceVariant_out2.vcf");
		COMPARE_FILES("out/VcfStoreSourceVariant_out2.vcf", TESTDATA("data_out/VcfStoreSourceVariant_out2.vcf"));
	}
};
