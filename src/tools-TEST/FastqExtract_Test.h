#include "TestFramework.h"

TEST_CLASS(FastqExtract_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("FastqExtract", "-in " + TESTDATA("data_in/FastqExtract_in1.fastq.gz") + " -ids " + TESTDATA("data_in/FastqExtract_in1.txt") + " -out out/FastqExtract_out1.fastq.gz");
		COMPARE_GZ_FILES("out/FastqExtract_out1.fastq.gz", TESTDATA("data_out/FastqExtract_out1.fastq.gz"));
	}

	void test_02()
	{
		EXECUTE("FastqExtract", "-in " + TESTDATA("data_in/FastqExtract_in2.fastq.gz") + " -ids " + TESTDATA("data_in/FastqExtract_in2.txt") + " -out out/FastqExtract_out2.fastq.gz");
		COMPARE_GZ_FILES("out/FastqExtract_out2.fastq.gz", TESTDATA("data_out/FastqExtract_out2.fastq.gz"));
	}
};
