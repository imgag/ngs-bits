#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedpeAnnotateBreakpointDensity_Test)
{
private:

	TEST_METHOD(std_params)
	{
		//test
		EXECUTE("BedpeAnnotateBreakpointDensity", "-in " + TESTDATA("data_in/BedpeAnnotateBreakpointDensity_in1.bedpe") + " -density " + TESTDATA("data_in/BedpeAnnotateBreakpointDensity_density.igv")
				+ " -out out/BedpeAnnotateBreakpointDensity_out1.bedpe");

		COMPARE_FILES("out/BedpeAnnotateBreakpointDensity_out1.bedpe", TESTDATA("data_out/BedpeAnnotateBreakpointDensity_out1.bedpe"));
	}

	TEST_METHOD(system_specific)
	{
		//test
		EXECUTE("BedpeAnnotateBreakpointDensity", "-in " + TESTDATA("data_in/BedpeAnnotateBreakpointDensity_in1.bedpe") + " -density " + TESTDATA("data_in/BedpeAnnotateBreakpointDensity_density.igv")
				+ " -density_sys " +  TESTDATA("data_in/BedpeAnnotateBreakpointDensity_density_sys.igv") +" -out out/BedpeAnnotateBreakpointDensity_out2.bedpe");

		COMPARE_FILES("out/BedpeAnnotateBreakpointDensity_out2.bedpe", TESTDATA("data_out/BedpeAnnotateBreakpointDensity_out2.bedpe"));
	}
};
