#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(BedpeExtractGenotype_Test)
{
private:

	TEST_METHOD(test_phased_annotation)
	{
		//test
		EXECUTE("BedpeExtractGenotype", "-in " + TESTDATA("data_in/BedpeExtractGenotype_in1.bedpe") + " -out out/BedpeExtractGenotype_out1.bedpe");

		COMPARE_FILES("out/BedpeExtractGenotype_out1.bedpe", TESTDATA("data_out/BedpeExtractGenotype_out1.bedpe"));
	}

	TEST_METHOD(test_unphased_annotation)
	{
		//test
		EXECUTE("BedpeExtractGenotype", "-in " + TESTDATA("data_in/BedpeExtractGenotype_in1.bedpe") + " -include_unphased -out out/BedpeExtractGenotype_out2.bedpe");

		COMPARE_FILES("out/BedpeExtractGenotype_out2.bedpe", TESTDATA("data_out/BedpeExtractGenotype_out2.bedpe"));
	}

	TEST_METHOD(test_unphased_reannotation)
	{
		//test
		EXECUTE("BedpeExtractGenotype", "-in " + TESTDATA("data_in/BedpeExtractGenotype_in2.bedpe") + " -include_unphased -out out/BedpeExtractGenotype_out3.bedpe");

		COMPARE_FILES("out/BedpeExtractGenotype_out3.bedpe", TESTDATA("data_out/BedpeExtractGenotype_out2.bedpe"));
	}

	TEST_METHOD(test_multisample)
	{
		//test
		EXECUTE("BedpeExtractGenotype", "-in " + TESTDATA("data_in/BedpeExtractGenotype_in3.bedpe") + " -include_unphased -out out/BedpeExtractGenotype_out4.bedpe");

		COMPARE_FILES("out/BedpeExtractGenotype_out4.bedpe", TESTDATA("data_out/BedpeExtractGenotype_out4.bedpe"));
	}
};
