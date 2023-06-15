#include "TestFramework.h"

TEST_CLASS(FastqTrim_Test)
{
Q_OBJECT
private slots:
	
	void start()
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out1.fastq.gz -start 5");
		COMPARE_GZ_FILES("out/FastqTrim_out1.fastq.gz", TESTDATA("data_out/FastqTrim_out1.fastq.gz"));
	}
	
	void start_end()
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out2.fastq.gz -start 5 -end 5");
		COMPARE_GZ_FILES("out/FastqTrim_out2.fastq.gz", TESTDATA("data_out/FastqTrim_out2.fastq.gz"));
	}

	void start_len()
	{
		EXECUTE("FastqTrim", "-in " + TESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out3.fastq.gz -start 5 -len 50");
		COMPARE_GZ_FILES("out/FastqTrim_out3.fastq.gz", TESTDATA("data_out/FastqTrim_out3.fastq.gz"));
	}

	void long_read()
	{
		EXECUTE("FastqTrim", "-long_read -in " + TESTDATA("data_in/FastqTrim_in2.fastq.gz") + " -out out/FastqTrim_out4.fastq.gz -len 1000");
		COMPARE_GZ_FILES("out/FastqTrim_out4.fastq.gz", TESTDATA("data_out/FastqTrim_out4.fastq.gz"));
	}

};
