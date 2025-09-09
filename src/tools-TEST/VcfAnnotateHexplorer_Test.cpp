#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfAnnotateHexplorer_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateHexplorer", "-in " + TESTDATA("data_in/VcfAnnotateHexplorer_in1.vcf") + " -out out/VcfAnnotateHexplorer_out1.vcf");
		COMPARE_FILES("out/VcfAnnotateHexplorer_out1.vcf", TESTDATA("data_out/VcfAnnotateHexplorer_out1.vcf"));
		VCF_IS_VALID("out/VcfAnnotateHexplorer_out1.vcf");
	}

};
