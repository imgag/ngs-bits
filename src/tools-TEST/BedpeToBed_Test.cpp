#include "TestFramework.h"

TEST_CLASS(BedpeToBed_Test)
{
private:

	TEST_METHOD(std_params)
	{
		//test
		EXECUTE("BedpeToBed", "-in " + TESTDATA("data_in/BedpeToBed_in1.bedpe") + " -out out/BedpeToBed_out1.bed");

		COMPARE_FILES("out/BedpeToBed_out1.bed", TESTDATA("data_out/BedpeToBed_out1.bed"));
	}

};
