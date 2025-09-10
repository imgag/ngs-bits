#include "TestFramework.h"

TEST_CLASS(FastqCheckUMI_Test)
{
private:
	
	TEST_METHOD(test_noUMI)
	{
		EXECUTE("FastqCheckUMI", "-in " + TESTDATA("data_in/FastqCheckUMI_in1.fastq.gz") + " -out out/FastqCheckUMI_out1.txt");
		COMPARE_FILES("out/FastqCheckUMI_out1.txt", TESTDATA("data_out/FastqCheckUMI_out1.txt"));
	}
	
	TEST_METHOD(test_UMI)
	{
		EXECUTE("FastqCheckUMI", "-in " + TESTDATA("data_in/FastqCheckUMI_in2.fastq.gz") + " -out out/FastqCheckUMI_out2.txt");
		COMPARE_FILES("out/FastqCheckUMI_out2.txt", TESTDATA("data_out/FastqCheckUMI_out2.txt"));
	}
	
	TEST_METHOD(test_novaseqx)
	{
		EXECUTE("FastqCheckUMI", "-in " + TESTDATA("data_in/FastqCheckUMI_in3.fastq.gz") + " -out out/FastqCheckUMI_out3.txt");
		COMPARE_FILES("out/FastqCheckUMI_out3.txt", TESTDATA("data_out/FastqCheckUMI_out3.txt"));
	}

};
