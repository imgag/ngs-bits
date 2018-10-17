#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfBreakmulti_TEST)
{
Q_OBJECT
private slots:
	void test_multi_allele()
	{
        EXECUTE("VcfBreakmulti", "-in " + TESTDATA("data_in/VcfBreakmulti_in1.vcf") + " -out out/VcfBreakmulti_out1.vcf");
        COMPARE_FILES("out/VcfBreakmulti_out1.vcf", TESTDATA("data_out/VcfBreakmulti_out1.vcf"));
		//TODO move VcfCheck to VcfFile class and use it after test to check that output is valid
		//TODO add test case with FLAG INFO
    }

	void test_no_allele()
	{
        // This should leave everything unchanged
        EXECUTE("VcfBreakmulti", "-in " + TESTDATA("data_in/VcfBreakmulti_in2.vcf") + " -out out/VcfBreakmulti_out2.vcf");
        COMPARE_FILES("out/VcfBreakmulti_out2.vcf", TESTDATA("data_out/VcfBreakmulti_out2.vcf"));
    }
};
