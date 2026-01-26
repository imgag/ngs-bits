#include "TestFramework.h"

TEST_CLASS(CnvFilterAnnotations_Test)
{
private:
	
    TEST_METHOD(empty_filter)
	{
		EXECUTE("CnvFilterAnnotations", "-in " + TESTDATA("data_in/CnvFilterAnnotations_in1.tsv") + " -filters " + TESTDATA("data_in/CnvFilterAnnotations_filters1.txt") + " -out out/CnvFilterAnnotations_out1.tsv");
		COMPARE_FILES("out/CnvFilterAnnotations_out1.tsv", TESTDATA("data_in/CnvFilterAnnotations_in1.tsv"));
	}

	TEST_METHOD(default_like_filter)
	{
		EXECUTE("CnvFilterAnnotations", "-in " + TESTDATA("data_in/CnvFilterAnnotations_in1.tsv") + " -filters " + TESTDATA("data_in/CnvFilterAnnotations_filters2.txt") + " -out out/CnvFilterAnnotations_out2.tsv");
		COMPARE_FILES("out/CnvFilterAnnotations_out2.tsv", TESTDATA("data_out/CnvFilterAnnotations_out2.tsv"));
	}

	TEST_METHOD(size_filter)
	{
		EXECUTE("CnvFilterAnnotations", "-in " + TESTDATA("data_in/CnvFilterAnnotations_in1.tsv") + " -filters " + TESTDATA("data_in/CnvFilterAnnotations_filters3.txt") + " -out out/CnvFilterAnnotations_out3.tsv");
		COMPARE_FILES("out/CnvFilterAnnotations_out3.tsv", TESTDATA("data_out/CnvFilterAnnotations_out3.tsv"));
	}

	TEST_METHOD(omim_filter)
	{
		EXECUTE("CnvFilterAnnotations", "-in " + TESTDATA("data_in/CnvFilterAnnotations_in1.tsv") + " -filters " + TESTDATA("data_in/CnvFilterAnnotations_filters4.txt") + " -out out/CnvFilterAnnotations_out4.tsv");
		COMPARE_FILES("out/CnvFilterAnnotations_out4.tsv", TESTDATA("data_out/CnvFilterAnnotations_out4.tsv"));
	}

};


