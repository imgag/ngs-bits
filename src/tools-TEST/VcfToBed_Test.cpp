#include "TestFramework.h"

TEST_CLASS(VcfToBed_Test)
{
private:

	TEST_METHOD(default_parameters)
	{
		EXECUTE("VcfToBed", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel_vep.vcf") + " -out out/VcfToBed_out01.bed");
		COMPARE_FILES("out/VcfToBed_out01.bed", TESTDATA("data_out/VcfToBed_out01.bed"));
	}
};
