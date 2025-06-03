#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfExtractSamples_Test)
{
Q_OBJECT
private slots:

	void one_sample()
    {
		EXECUTE("VcfExtractSamples", "-in " + TESTDATA("data_in/VcfExtractSamples_in1.vcf") + " -out out/VcfExtractSamples_out1.vcf" + " -samples Sample1");
		COMPARE_FILES("out/VcfExtractSamples_out1.vcf", TESTDATA("data_out/VcfExtractSamples_out1.vcf"));
		VCF_IS_VALID_HG19("out/VcfExtractSamples_out1.vcf");
    }

	void two_sample()
	{
		EXECUTE("VcfExtractSamples", "-in " + TESTDATA("data_in/VcfExtractSamples_in1.vcf") + " -out out/VcfExtractSamples_out2.vcf" + " -samples Sample2,Sample1");
		COMPARE_FILES("out/VcfExtractSamples_out2.vcf", TESTDATA("data_out/VcfExtractSamples_out2.vcf"));
		VCF_IS_VALID_HG19("out/VcfExtractSamples_out2.vcf");
	}
};
