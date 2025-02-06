#include "TestFramework.h"


TEST_CLASS(MantaVcfFix_Test)
{
Q_OBJECT
private slots:
	
	void manta_test()
	{
		EXECUTE("MantaVcfFix", "-in " + TESTDATA("data_in/MantaVcfFix_in1.vcf.gz") + " -out out/MantaVcfFix_out1.vcf");
		COMPARE_FILES("out/MantaVcfFix_out1.vcf", TESTDATA("data_out/MantaVcfFix_out1.vcf"));
	}
	
	void dragen_test()
	{
		EXECUTE("MantaVcfFix", "-in " + TESTDATA("data_in/MantaVcfFix_in2.vcf") + " -out out/MantaVcfFix_out2.vcf");
		COMPARE_FILES("out/MantaVcfFix_out2.vcf", TESTDATA("data_out/MantaVcfFix_out2.vcf"));
	}
};

