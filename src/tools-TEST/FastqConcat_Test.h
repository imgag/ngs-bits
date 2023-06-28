#include "TestFramework.h"

TEST_CLASS(FastqConcat_Test)
{
Q_OBJECT
private slots:
	
	void default_test()
	{
		EXECUTE("FastqConcat", "-in " + TESTDATA("data_in/FastqConcat_in1.fastq.gz") + " "
				+ TESTDATA("data_in/FastqConcat_in2.fastq.gz") + " "
				+ TESTDATA("data_in/FastqConcat_in3.fastq.gz")
				+ " -out out/FastqConcat_out.fastq.gz");
		COMPARE_GZ_FILES("out/FastqConcat_out.fastq.gz", TESTDATA("data_out/FastqConcat_out.fastq.gz"));
	}

	void long_test()
	{
		EXECUTE("FastqConcat", "-long_read -in " + TESTDATA("data_in/FastqConcat_in4.fastq.gz") + " "
				+ TESTDATA("data_in/FastqConcat_in5.fastq.gz")
				+ " -out out/FastqConcat_out2.fastq.gz");
		COMPARE_GZ_FILES("out/FastqConcat_out2.fastq.gz", TESTDATA("data_out/FastqConcat_out2.fastq.gz"));
	}

};
