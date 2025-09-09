#include "TestFramework.h"

TEST_CLASS( FastqExtractUMI_Test)
{
private:

	TEST_METHOD(test_01)
	{
		EXECUTE( "FastqExtractUMI", "-cut1 12 -in1 " +
				 TESTDATA("data_in/FastqExtractBarcode_in1.fastq.gz") +
				 " -in2 " +
				 TESTDATA("data_in/FastqExtractBarcode_in2.fastq.gz") +
				 " -out1 out/FastqExtractUMI_out1.fastq.gz -out2 out/FastqExtractUMI_out2.fastq.gz");
		COMPARE_GZ_FILES("out/FastqExtractUMI_out1.fastq.gz", TESTDATA("data_out/FastqExtractUMI_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/FastqExtractUMI_out2.fastq.gz", TESTDATA("data_out/FastqExtractUMI_out2.fastq.gz"));
	}

	TEST_METHOD(test_02)
	{
		EXECUTE( "FastqExtractUMI", "-cut1 8 -cut2 8 -in1 " +
				 TESTDATA("data_in/FastqExtractBarcode_in1.fastq.gz") +
				 " -in2 " +
				 TESTDATA("data_in/FastqExtractBarcode_in2.fastq.gz") +
				 " -out1 out/FastqExtractUMI_out3.fastq.gz -out2 out/FastqExtractUMI_out4.fastq.gz");
		COMPARE_GZ_FILES("out/FastqExtractUMI_out3.fastq.gz", TESTDATA("data_out/FastqExtractUMI_out3.fastq.gz"));
		COMPARE_GZ_FILES("out/FastqExtractUMI_out4.fastq.gz", TESTDATA("data_out/FastqExtractUMI_out4.fastq.gz"));
	}
};
