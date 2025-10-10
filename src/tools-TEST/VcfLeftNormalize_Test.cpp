#include "TestFramework.h"
#include "Settings.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfLeftNormalize_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfLeftNormalize", "-in " + TESTDATA("data_in/VcfLeftNormalize_in1.vcf") + " -out out/VcfLeftNormalize_out1.vcf -ref " + ref_file);
		COMPARE_FILES("out/VcfLeftNormalize_out1.vcf", TESTDATA("data_out/VcfLeftNormalize_out1.vcf"));
		VCF_IS_VALID("out/VcfLeftNormalize_out1.vcf")
	}

	TEST_METHOD(test_02_stream)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfLeftNormalize", "-stream -ref " + ref_file + " -in " +TESTDATA("data_in/VcfLeftNormalize_in1.vcf") + " -out out/VcfLeftNormalize_out2.vcf")
		COMPARE_FILES("out/VcfLeftNormalize_out2.vcf", TESTDATA("data_out/VcfLeftNormalize_out2.vcf"));
		VCF_IS_VALID("out/VcfLeftNormalize_out2.vcf")
	}

	TEST_METHOD(test_03_right)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfLeftNormalize", "-right -in " + TESTDATA("data_in/VcfLeftNormalize_in3.vcf") + " -out out/VcfLeftNormalize_out3.vcf -ref " + ref_file);
		COMPARE_FILES("out/VcfLeftNormalize_out3.vcf", TESTDATA("data_out/VcfLeftNormalize_out3.vcf"));
		VCF_IS_VALID("out/VcfLeftNormalize_out3.vcf")
	}

	TEST_METHOD(test_04_right_stream)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfLeftNormalize", "-right -stream -in " + TESTDATA("data_in/VcfLeftNormalize_in3.vcf") + " -out out/VcfLeftNormalize_out4.vcf -ref " + ref_file);
		COMPARE_FILES("out/VcfLeftNormalize_out4.vcf", TESTDATA("data_out/VcfLeftNormalize_out4.vcf"));
		VCF_IS_VALID("out/VcfLeftNormalize_out4.vcf")
	}

};

