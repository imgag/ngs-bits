#include "TestFramework.h"

TEST_CLASS(TsvInfo_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("TsvInfo", "-in " + TESTDATA("data_in/TsvInfo_in1.tsv") + " -out out/TsvInfo_out1.txt");
		COMPARE_FILES("out/TsvInfo_out1.txt", TESTDATA("data_out/TsvInfo_out1.txt"));
	}

	TEST_METHOD(test_02)
	{
		EXECUTE("TsvInfo", "-in " + TESTDATA("data_in/TsvInfo_in2.tsv") + " -out out/TsvInfo_out2.txt");
		COMPARE_FILES("out/TsvInfo_out2.txt", TESTDATA("data_out/TsvInfo_out2.txt"));
	}
};

