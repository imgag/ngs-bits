#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VariantAnnotateFrequency_Test)
{
private:

	//Test with name and depth arguments
	TEST_METHOD(gsvar_with_depth_and_name)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out1.tsv -depth -name bla -ref " + ref_file);
		COMPARE_FILES("out/VariantAnnotateFrequency_out1.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out1.tsv"));
	}
	

	//Test without arguments
	TEST_METHOD(gsvar_no_arguments)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out2.tsv -ref " + ref_file);
		COMPARE_FILES("out/VariantAnnotateFrequency_out2.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out2.tsv"));
	}

	//Test with mapq0 argument
	TEST_METHOD(gsvar_mapq0)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in2.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out3.tsv -mapq0 -ref " + ref_file);
		COMPARE_FILES("out/VariantAnnotateFrequency_out3.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out3.tsv"));
	}

};


