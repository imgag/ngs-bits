#include "TestFramework.h"

TEST_CLASS(BedpeNormalize_Test)
{
private:

	TEST_METHOD(manta)
	{
		EXECUTE("BedpeNormalize", "-in " + TESTDATA("data_in/BedpeNormalize_in1_manta.bedpe") + " -out out/BedpeNormalize_out1_manta.bedpe");
		COMPARE_FILES("out/BedpeNormalize_out1_manta.bedpe", TESTDATA("data_out/BedpeNormalize_out1_manta.bedpe"));
	}

	TEST_METHOD(dragen)
	{
		EXECUTE("BedpeNormalize", "-in " + TESTDATA("data_in/BedpeNormalize_in2_dragen.bedpe") + " -out out/BedpeNormalize_out2_dragen.bedpe");
		COMPARE_FILES("out/BedpeNormalize_out2_dragen.bedpe", TESTDATA("data_out/BedpeNormalize_out2_dragen.bedpe"));
	}

	TEST_METHOD(sniffles)
	{
		EXECUTE("BedpeNormalize", "-in " + TESTDATA("data_in/BedpeNormalize_in3_sniffles.bedpe") + " -out out/BedpeNormalize_out3_sniffles.bedpe");
		COMPARE_FILES("out/BedpeNormalize_out3_sniffles.bedpe", TESTDATA("data_out/BedpeNormalize_out3_sniffles.bedpe"));
	}

};
