#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfAdd_Test)
{
Q_OBJECT
private slots:
	
	void default_mode()
	{
		EXECUTE("VcfAdd", "-in " + TESTDATA("data_in/VcfAdd_in1.vcf") + " " + TESTDATA("data_in/VcfAdd_in2.vcf") + " -out out/VcfAdd_out1.vcf");
		COMPARE_FILES("out/VcfAdd_out1.vcf", TESTDATA("data_out/VcfAdd_out1.vcf"));
		VCF_IS_VALID_HG19("out/VcfAdd_out1.vcf");
	}

	void with_filters()
	{
		EXECUTE("VcfAdd", "-in " + TESTDATA("data_in/VcfAdd_in1.vcf") + " " + TESTDATA("data_in/VcfAdd_in2.vcf") + " -filter mosaic -filter_desc bli_bla_bluff. -out out/VcfAdd_out2.vcf");
		COMPARE_FILES("out/VcfAdd_out2.vcf", TESTDATA("data_out/VcfAdd_out2.vcf"));
		VCF_IS_VALID_HG19("out/VcfAdd_out2.vcf");
	}

	void with_filters_and_skip_duplicates()
	{
		EXECUTE("VcfAdd", "-in " + TESTDATA("data_in/VcfAdd_in1.vcf") + " " + TESTDATA("data_in/VcfAdd_in2.vcf") + " -filter mosaic -filter_desc bli_bla_bluff. -skip_duplicates -out out/VcfAdd_out3.vcf");
		COMPARE_FILES("out/VcfAdd_out3.vcf", TESTDATA("data_out/VcfAdd_out3.vcf"));
		VCF_IS_VALID_HG19("out/VcfAdd_out3.vcf");
	}
};
