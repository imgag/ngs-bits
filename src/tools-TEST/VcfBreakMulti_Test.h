#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfBreakMulti_Test)
{
Q_OBJECT
private slots:
	void single_sample()
	{
		EXECUTE("VcfBreakMulti", "-in " + TESTDATA("data_in/VcfBreakMulti_in1.vcf") + " -out out/VcfBreakMulti_out1.vcf");
		COMPARE_FILES("out/VcfBreakMulti_out1.vcf", TESTDATA("data_out/VcfBreakMulti_out1.vcf"));
		VCF_IS_VALID_HG19("out/VcfBreakMulti_out1.vcf")
    }

	void multi_sample()
	{
		EXECUTE("VcfBreakMulti", "-in " + TESTDATA("data_in/VcfBreakMulti_in2.vcf") + " -out out/VcfBreakMulti_out2.vcf");
		COMPARE_FILES("out/VcfBreakMulti_out2.vcf", TESTDATA("data_out/VcfBreakMulti_out2.vcf"));
		VCF_IS_VALID_HG19("out/VcfBreakMulti_out2.vcf")
	}

	void no_sample()
	{
		EXECUTE("VcfBreakMulti", "-in " + TESTDATA("data_in/VcfBreakMulti_in3.vcf") + " -out out/VcfBreakMulti_out3.vcf");
		COMPARE_FILES("out/VcfBreakMulti_out3.vcf", TESTDATA("data_out/VcfBreakMulti_out3.vcf"));
		VCF_IS_VALID("out/VcfBreakMulti_out3.vcf")
	}
};
