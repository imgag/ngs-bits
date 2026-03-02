#include "TestFramework.h"

TEST_CLASS(FastaChecksumUpdate_Test)
{
private:
	TEST_METHOD(cheksum_update_test)
	{
		EXECUTE("FastaChecksumUpdate", "-in " + TESTDATA("data_in/FastaChecksumUpdate_in1.fa") + " -out out/FastaChecksumUpdate_out1.fa");
		COMPARE_FILES("out/FastaChecksumUpdate_out1.fa", TESTDATA("data_out/FastaChecksumUpdate_out1.fa"));
	}
};
