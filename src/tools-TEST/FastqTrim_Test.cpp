#include "TestFramework.h"

TEST_CLASS(FastqTrim_Test)
{
private:
	
	TEST_METHOD(start)
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out1.fastq.gz -start 5");
		COMPARE_GZ_FILES("out/FastqTrim_out1.fastq.gz", TESTDATA("data_out/FastqTrim_out1.fastq.gz"));
	}
	
	TEST_METHOD(start_end)
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out2.fastq.gz -start 5 -end 5");
		COMPARE_GZ_FILES("out/FastqTrim_out2.fastq.gz", TESTDATA("data_out/FastqTrim_out2.fastq.gz"));
	}

	TEST_METHOD(start_len)
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out3.fastq.gz -start 5 -len 50");
		COMPARE_GZ_FILES("out/FastqTrim_out3.fastq.gz", TESTDATA("data_out/FastqTrim_out3.fastq.gz"));
	}

	TEST_METHOD(max_len)
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out4.fastq.gz -end 5 -max_len 80");
		COMPARE_GZ_FILES("out/FastqTrim_out4.fastq.gz", TESTDATA("data_out/FastqTrim_out4.fastq.gz"));
	}

	TEST_METHOD(all)
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out5.fastq.gz -len 50 -start 5 -end 5 -max_len 80");
		COMPARE_GZ_FILES("out/FastqTrim_out5.fastq.gz", TESTDATA("data_out/FastqTrim_out5.fastq.gz"));
	}

	TEST_METHOD(long_read)
	{
		EXECUTE("FastqTrim", "-long_read -in " + TESTDATA("data_in/FastqTrim_in2.fastq.gz") + " -out out/FastqTrim_out6.fastq.gz -len 1000");
		COMPARE_GZ_FILES("out/FastqTrim_out6.fastq.gz", TESTDATA("data_out/FastqTrim_out6.fastq.gz"));
	}

};
