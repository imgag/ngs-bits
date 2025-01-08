#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfSubtract_Test)
{
Q_OBJECT
private slots:
	
	void default_mode()
	{
		EXECUTE("VcfSubtract", "-in " + TESTDATA("data_in/VcfSubtract_in1.vcf") + " -in2 " + TESTDATA("data_in/VcfSubtract_in2.vcf") + " -out out/VcfSubtract_out1.vcf");
		COMPARE_FILES("out/VcfSubtract_out1.vcf", TESTDATA("data_out/VcfSubtract_out1.vcf"));
		VCF_IS_VALID_HG19("out/VcfSubtract_out1.vcf");
	}
};
