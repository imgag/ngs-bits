#include "TestFramework.h"

TEST_CLASS(FastqAddBarcode_Test)
{
private:

	TEST_METHOD(test_01)
	{
		EXECUTE("FastqAddBarcode", "-in1 " +
				TESTDATA("data_in/FastqExtractBarcode_in1.fastq.gz") +
				" -in2 " +
				TESTDATA("data_in/FastqExtractBarcode_in2.fastq.gz") +
				" -in_barcode " +
				TESTDATA("data_in/FastqAddBarcode_in.fastq.gz") +
				" -out1 out/FastqAddBarcode_out1.fastq.gz -out2 out/FastqAddBarcode_out2.fastq.gz");
		COMPARE_GZ_FILES("out/FastqAddBarcode_out1.fastq.gz", TESTDATA("data_out/FastqAddBarcode_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/FastqAddBarcode_out2.fastq.gz", TESTDATA("data_out/FastqAddBarcode_out2.fastq.gz"));
	}

};
