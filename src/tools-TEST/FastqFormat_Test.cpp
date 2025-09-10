#include "TestFramework.h"

TEST_CLASS(FastqFormat_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("FastqFormat", "-in " + TESTDATA("data_in/FastqFormat_in1.fastq") + " -out out/FastqFormat_out1.txt");
		COMPARE_FILES("out/FastqFormat_out1.txt", TESTDATA("data_out/FastqFormat_out1.txt"));
	}
	
	TEST_METHOD(test_02)
	{
		EXECUTE("FastqFormat", "-in " + TESTDATA("data_in/FastqFormat_in2.fastq.gz") + " -out out/FastqFormat_out2.txt");
		COMPARE_FILES("out/FastqFormat_out2.txt", TESTDATA("data_out/FastqFormat_out2.txt"));
	}

};
