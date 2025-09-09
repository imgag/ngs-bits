#include "TestFramework.h"

TEST_CLASS(VariantFilterAnnotations_Test)
{
private:
	
	TEST_METHOD(no_filter)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters1.txt") + " -out out/VariantFilterAnnotations_out1.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out1.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out1.GSvar"));
	}

	TEST_METHOD(maxAf)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters2.txt") + " -out out/VariantFilterAnnotations_out2.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out2.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out2.GSvar"));
	}

	TEST_METHOD(impact_ihdb)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters3.txt") + " -out out/VariantFilterAnnotations_out3.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out3.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out3.GSvar"));
	}

	TEST_METHOD(class_filters_genoAffected)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters4.txt") + " -out out/VariantFilterAnnotations_out4.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out4.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out4.GSvar"));
	}

	TEST_METHOD(maxSubAf)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters5.txt") + " -out out/VariantFilterAnnotations_out5.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out5.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out5.GSvar"));
	}

	TEST_METHOD(multisample_maxAf_dominant)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters6.txt") + " -out out/VariantFilterAnnotations_out6.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out6.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out6.GSvar"));
	}

	TEST_METHOD(multisample_maxAf_recessive)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters7.txt") + " -out out/VariantFilterAnnotations_out7.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out7.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out7.GSvar"));
	}

	TEST_METHOD(multisample_notWT)
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters8.txt") + " -out out/VariantFilterAnnotations_out8.GSvar");
		COMPARE_FILES("out/VariantFilterAnnotations_out8.GSvar", TESTDATA("data_out/VariantFilterAnnotations_out8.GSvar"));
	}

};


