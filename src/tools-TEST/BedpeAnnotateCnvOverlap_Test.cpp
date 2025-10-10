#include "TestFramework.h"

TEST_CLASS(BedpeAnnotateCnvOverlap_Test)
{
private:

	TEST_METHOD(test_01)
	{
		//test
		EXECUTE("BedpeAnnotateCnvOverlap", "-in " + TESTDATA("data_in/BedpeAnnotateCnvOverlap_in1.bedpe") + " -cnv "
				+ TESTDATA("data_in/BedpeAnnotateCnvOverlap_in_cnv.tsv") + " -out out/BedpeAnnotateCnvOverlap_out1.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCnvOverlap_out1.bedpe", TESTDATA("data_out/BedpeAnnotateCnvOverlap_out1.bedpe"));
	}
};
