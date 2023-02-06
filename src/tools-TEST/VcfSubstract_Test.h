#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfSubstract_Test)
{
Q_OBJECT
private slots:
	
	void default_mode()
	{
		EXECUTE("VcfSubstract", "-in " + TESTDATA("data_in/VcfSubstract_in1.vcf") + " -in2 " + TESTDATA("data_in/VcfSubstract_in2.vcf") + " -out out/VcfSubstract_out1.vcf");
		COMPARE_FILES("out/VcfSubstract_out1.vcf", TESTDATA("data_out/VcfSubstract_out1.vcf"));
		VCF_IS_VALID("out/VcfSubstract_out1.vcf");
	}
};
