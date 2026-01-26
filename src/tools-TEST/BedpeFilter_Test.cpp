#include "TestFramework.h"

TEST_CLASS(BedpeFilter_Test)
{
private:

	TEST_METHOD(filter_roi)
	{
		//test
		EXECUTE("BedpeFilter", "-in " + TESTDATA("data_in/BedpeFilter_in1.bedpe") + " -bed " + TESTDATA("data_in/BedpeFilter_in.bed")
				+ " -out out/BedpeFilter_out1.bedpe");
		COMPARE_FILES("out/BedpeFilter_out1.bedpe", TESTDATA("data_out/BedpeFilter_out1.bedpe"));
	}

};
