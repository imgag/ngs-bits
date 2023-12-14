#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfMerge_Test)
{
Q_OBJECT
private slots:
	
	void default_test()
	{
		EXECUTE_FAIL("VcfMerge", "-in " + TESTDATA("data_in/VcfMerge_in1.vcf") + " " + TESTDATA("data_in/VcfMerge_in2.vcf") + " -out out/VcfMerge_out1.vcf");
		COMPARE_FILES("out/VcfMerge_out1.vcf", TESTDATA("data_out/VcfMerge_out1.vcf"));
	}

	void gz_input_test()
	{
		EXECUTE_FAIL("VcfMerge", "-in " + TESTDATA("data_in/VcfMerge_in1.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_in2.vcf.gz") + " -out out/VcfMerge_out2.vcf");
		COMPARE_FILES("out/VcfMerge_out2.vcf", TESTDATA("data_out/VcfMerge_out1.vcf"));
	}
};

