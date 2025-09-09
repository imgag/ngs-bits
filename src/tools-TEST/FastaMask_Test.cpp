#include "TestFramework.h"

TEST_CLASS(FastaMask_Test)
{
private:
	
	TEST_METHOD(basic_test)
	{
		EXECUTE("FastaMask", "-in " + TESTDATA("data_in/FastaMask_in1.fa") + " -reg " + TESTDATA("data_in/FastaMask_in1.bed") + " -out out/FastaMask_out1.fa");
		COMPARE_FILES("out/FastaMask_out1.fa", TESTDATA("data_out/FastaMask_out1.fa"));
	}

};
