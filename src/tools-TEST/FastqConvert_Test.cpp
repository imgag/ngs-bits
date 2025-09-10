#include "TestFramework.h"

TEST_CLASS(FastqConvert_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("FastqConvert", "-in " + TESTDATA("data_in/FastqConvert_in1.fastq.gz") + " -out out/FastqConvert_out1.fastq.gz");
		COMPARE_GZ_FILES("out/FastqConvert_out1.fastq.gz", TESTDATA("data_out/FastqConvert_out1.fastq.gz"));
	}

};
