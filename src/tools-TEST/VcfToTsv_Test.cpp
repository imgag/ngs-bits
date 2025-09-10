#include "TestFramework.h"

TEST_CLASS(VcfToTsv_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		EXECUTE("VcfToTsv", "-in " + TESTDATA("data_in/VcfToTsv_in1.vcf") + " -out out/VcfToTsv_out1.tsv");
		COMPARE_FILES("out/VcfToTsv_out1.tsv", TESTDATA("data_out/VcfToTsv_out1.tsv"));
	}

    TEST_METHOD(multi_allelic_multi_sample)
    {
		EXECUTE("VcfToTsv", "-in " + TESTDATA("data_in/VcfToTsv_in2.vcf") + " -out out/VcfToTsv_out2.tsv");
        COMPARE_FILES("out/VcfToTsv_out2.tsv", TESTDATA("data_out/VcfToTsv_out2.tsv"));
    }
};

