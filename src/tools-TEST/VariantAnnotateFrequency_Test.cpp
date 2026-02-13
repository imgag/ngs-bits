#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VariantAnnotateFrequency_Test)
{
private:

	//Test with name and depth arguments
	TEST_METHOD(gsvar_with_depth_and_name)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out1.tsv -depth -name bla -ref " + ref_file);
		COMPARE_FILES("out/VariantAnnotateFrequency_out1.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out1.tsv"));
	}
	

	//Test without arguments
	TEST_METHOD(gsvar_no_arguments)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

        EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out2.tsv -ref " + ref_file);
		COMPARE_FILES("out/VariantAnnotateFrequency_out2.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out2.tsv"));
	}

	//Test with mapq0 argument
	TEST_METHOD(gsvar_mapq0)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

        EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in2.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out3.tsv -mapq0 -ref " + ref_file);
		COMPARE_FILES("out/VariantAnnotateFrequency_out3.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out3.tsv"));
	}

    //Test with fragments argument
    TEST_METHOD(gsvar_fragment)
    {
        SKIP_IF_NO_HG38_GENOME();

        QString ref_file = Settings::string("reference_genome", true);

        EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out4.tsv -name bla -depth -fragments -ref " + ref_file);
        COMPARE_FILES("out/VariantAnnotateFrequency_out4.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out4.tsv"));
    }

    //Test with target argument
    TEST_METHOD(gsvar_target)
    {
        SKIP_IF_NO_HG38_GENOME();

        QString ref_file = Settings::string("reference_genome", true);

        EXECUTE("VariantAnnotateFrequency", "-in " + TESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out5.tsv -name bla -target " + TESTDATA("data_in/VariantAnnotateFrequency_target.bed") + " -depth -ref " + ref_file);
        COMPARE_FILES("out/VariantAnnotateFrequency_out5.tsv", TESTDATA("data_out/VariantAnnotateFrequency_out5.tsv"));
    }

};


