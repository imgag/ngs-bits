#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfAnnotateFrequency_Test)
{
private:

	//Test with name and depth arguments
	TEST_METHOD(with_depth_and_name)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfAnnotateFrequency", "-in " + TESTDATA("data_in/VcfAnnotateFrequency_in1.vcf") + " -bam " + TESTDATA("data_in/VcfAnnotateFrequency_in1.bam") + " -out out/VcfAnnotateFrequency_out1.vcf -depth -name TEST_SAMPLE_01 -ref " + ref_file);
		COMPARE_FILES("out/VcfAnnotateFrequency_out1.vcf", TESTDATA("data_out/VcfAnnotateFrequency_out1.vcf"));
	}
	

	//Test without arguments
	TEST_METHOD(no_arguments)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfAnnotateFrequency", "-in " + TESTDATA("data_in/VcfAnnotateFrequency_in1.vcf") + " -bam " + TESTDATA("data_in/VcfAnnotateFrequency_in1.bam") + " -out out/VcfAnnotateFrequency_out2.vcf -ref " + ref_file);
		COMPARE_FILES("out/VcfAnnotateFrequency_out2.vcf", TESTDATA("data_out/VcfAnnotateFrequency_out2.vcf"));
	}

};


