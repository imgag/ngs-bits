#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfPruneSourceVariant_Test)
{
private:

	TEST_METHOD(default_mode)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfPruneSourceVariant", "-in " + TESTDATA("data_in/VcfPruneSourceVariant_in1.vcf") + " -out out/VcfPruneSourceVariant_out1.vcf -ref " + ref_file);
		COMPARE_FILES("out/VcfPruneSourceVariant_out1.vcf", TESTDATA("data_out/VcfPruneSourceVariant_out1.vcf"));
	}

};
