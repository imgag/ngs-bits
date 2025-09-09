#include "TestFramework.h"

TEST_CLASS(SvFilterAnnotations_Test)
{
private:
	
    TEST_METHOD(empty_filter)
	{
        EXECUTE("SvFilterAnnotations", "-in " + TESTDATA("data_in/SvFilterAnnotations_in1.bedpe") + " -filters " + TESTDATA("data_in/SvFilterAnnotations_filters1.txt") + " -out out/SvFilterAnnotations_out1.bedpe");
        COMPARE_FILES("out/SvFilterAnnotations_out1.bedpe", TESTDATA("data_in/SvFilterAnnotations_in1.bedpe"));
	}

    TEST_METHOD(omim_filter)
    {
        EXECUTE("SvFilterAnnotations", "-in " + TESTDATA("data_in/SvFilterAnnotations_in1.bedpe") + " -filters " + TESTDATA("data_in/SvFilterAnnotations_filters2.txt") + " -out out/SvFilterAnnotations_out2.bedpe");
        COMPARE_FILES("out/SvFilterAnnotations_out2.bedpe", TESTDATA("data_out/SvFilterAnnotations_out2.bedpe"));
    }

    TEST_METHOD(specChr_filter)
    {
        EXECUTE("SvFilterAnnotations", "-in " + TESTDATA("data_in/SvFilterAnnotations_in1.bedpe") + " -filters " + TESTDATA("data_in/SvFilterAnnotations_filters3.txt") + " -out out/SvFilterAnnotations_out3.bedpe");
        COMPARE_FILES("out/SvFilterAnnotations_out3.bedpe", TESTDATA("data_out/SvFilterAnnotations_out3.bedpe"));
    }

    TEST_METHOD(specChr_omim_filter)
    {
        EXECUTE("SvFilterAnnotations", "-in " + TESTDATA("data_in/SvFilterAnnotations_in1.bedpe") + " -filters " + TESTDATA("data_in/SvFilterAnnotations_filters4.txt") + " -out out/SvFilterAnnotations_out4.bedpe");
        COMPARE_FILES("out/SvFilterAnnotations_out4.bedpe", TESTDATA("data_out/SvFilterAnnotations_out4.bedpe"));
    }


};


