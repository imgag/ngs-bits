#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfStrip_Test)
{
Q_OBJECT
private slots:
	void default_test()
	{
		EXECUTE("VcfStrip", "-in " + TESTDATA("data_in/VcfStrip_in1.vcf") + " -out out/VcfStrip_out1.vcf -info AF,PQR,SRR,AB,PQA -format GT,AD,QR,QA");
		COMPARE_FILES("out/VcfStrip_out1.vcf", TESTDATA("data_out/VcfStrip_out1.vcf"));
		VCF_IS_VALID_HG19("out/VcfStrip_out1.vcf")
    }

	void no_info_test()
	{
		EXECUTE("VcfStrip", "-in " + TESTDATA("data_in/VcfStrip_in1.vcf") + " -out out/VcfStrip_out2.vcf -format GT,DP,AO");
		COMPARE_FILES("out/VcfStrip_out2.vcf", TESTDATA("data_out/VcfStrip_out2.vcf"));
		VCF_IS_VALID_HG19("out/VcfStrip_out2.vcf")
	}
};
