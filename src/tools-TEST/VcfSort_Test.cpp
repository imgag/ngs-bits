#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfSort_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in1.vcf") + " -out out/VcfSort_out1.vcf");
		COMPARE_FILES("out/VcfSort_out1.vcf", TESTDATA("data_out/VcfSort_out1.vcf"));
		VCF_IS_VALID(TESTDATA("data_out/VcfSort_out1.vcf"));
	}

	TEST_METHOD(split_chrs)
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in1.vcf") + " -split_chrs -out out/VcfSort_out2.vcf");
		COMPARE_FILES("out/VcfSort_out2.vcf", TESTDATA("data_out/VcfSort_out2.vcf"));
		VCF_IS_VALID(TESTDATA("data_out/VcfSort_out2.vcf"));
	}

	TEST_METHOD(remove_unused_contigs)
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in1.vcf") + " -remove_unused_contigs -out out/VcfSort_out3.vcf");
		COMPARE_FILES("out/VcfSort_out3.vcf", TESTDATA("data_out/VcfSort_out3.vcf"));
		VCF_IS_VALID(TESTDATA("data_out/VcfSort_out3.vcf"));
	}

	TEST_METHOD(compression_level)
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in1.vcf") + " -compression_level 5 -out out/VcfSort_out4.vcf.gz");
		COMPARE_FILES("out/VcfSort_out4.vcf.gz", TESTDATA("data_out/VcfSort_out4.vcf.gz"));
		VCF_IS_VALID(TESTDATA("data_out/VcfSort_out4.vcf.gz"));
	}

	TEST_METHOD(bug_GT_not_first_format_field)
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in2.vcf") + " -out out/VcfSort_out5.vcf");
		COMPARE_FILES("out/VcfSort_out5.vcf", TESTDATA("data_out/VcfSort_out5.vcf"));
		VCF_IS_VALID(TESTDATA("data_out/VcfSort_out5.vcf"));
	}
};


