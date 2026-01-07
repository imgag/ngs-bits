#include "TestFramework.h"

TEST_CLASS(VcfStoreSourceVariant_Test)
{
private:
	
	TEST_METHOD(reannotate)
	{
		EXECUTE("VcfStoreSourceVariant", "-in " + TESTDATA("data_in/VcfStoreSourceVariant_in1.vcf") + " -out out/VcfStoreSourceVariant_out1.vcf");
		COMPARE_FILES("out/VcfStoreSourceVariant_out1.vcf", TESTDATA("data_out/VcfStoreSourceVariant_out1.vcf"));
	}

	TEST_METHOD(first_annotation)
	{
		EXECUTE("VcfStoreSourceVariant", "-in " + TESTDATA("data_in/VcfStoreSourceVariant_in2.vcf") + " -out out/VcfStoreSourceVariant_out2.vcf");
		COMPARE_FILES("out/VcfStoreSourceVariant_out2.vcf", TESTDATA("data_out/VcfStoreSourceVariant_out1.vcf"));
	}
};
