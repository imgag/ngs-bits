#include "TestFramework.h"

TEST_CLASS(FastqList_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("FastqList", "-in " + TESTDATA("data_in/FastqList_in1.fastq.gz") + " -out out/FastqList_out1.txt");
		COMPARE_GZ_FILES("out/FastqList_out1.txt", TESTDATA("data_out/FastqList_out1.txt"));
	}

};
