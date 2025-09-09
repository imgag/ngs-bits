#include "TestFramework.h"

TEST_CLASS(FastqMidParser_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("FastqMidParser", "-in " + TESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out1.txt");
		COMPARE_FILES("out/FastqMidParser_out1.txt", TESTDATA("data_out/FastqMidParser_out1.txt"));
	}
	
	TEST_METHOD(test_02)
	{
		EXECUTE("FastqMidParser", "-in " + TESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out2.txt -lines 500 -mids 5");
		COMPARE_FILES("out/FastqMidParser_out2.txt", TESTDATA("data_out/FastqMidParser_out2.txt"));
	}
	
	TEST_METHOD(test_03)
	{
		EXECUTE("FastqMidParser", "-in " + TESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out3.txt -sheet " + TESTDATA("data_in/FastqMidParser_in1.csv"));
		COMPARE_FILES("out/FastqMidParser_out3.txt", TESTDATA("data_out/FastqMidParser_out3.txt"));
	}

};
