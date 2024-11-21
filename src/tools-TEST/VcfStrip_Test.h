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
		EXECUTE("VcfStrip", "-in " + TESTDATA("data_in/VcfStrip_in1.vcf") + " -out out/VcfStrip_out2.vcf -format GT,DP,AO -clear_info");
		COMPARE_FILES("out/VcfStrip_out2.vcf", TESTDATA("data_out/VcfStrip_out2.vcf"));
		VCF_IS_VALID_HG19("out/VcfStrip_out2.vcf")
	}

	void no_stripping_test()
	{
		EXECUTE("VcfStrip", "-in " + TESTDATA("data_in/VcfStrip_in1.vcf") + " -out out/VcfStrip_out3.vcf");
		COMPARE_FILES("out/VcfStrip_out3.vcf", TESTDATA("data_out/VcfStrip_out3.vcf"));
		VCF_IS_VALID_HG19("out/VcfStrip_out3.vcf")
	}

	void key_only_info_test()
	{
		EXECUTE("VcfStrip", "-in " + TESTDATA("data_in/VcfStrip_in1.vcf") + " -out out/VcfStrip_out4.vcf -info DP,LEN,SOME_FLAG -format GT,DP,QA");
		COMPARE_FILES("out/VcfStrip_out4.vcf", TESTDATA("data_out/VcfStrip_out4.vcf"));
		VCF_IS_VALID_HG19("out/VcfStrip_out4.vcf")
	}
};
