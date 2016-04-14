#include "TestFramework.h"

TEST_CLASS( FastqExtractBarcode_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		EXECUTE( "FastqExtractBarcode", "-in " + TESTDATA("data_in/FastqExtractBarcode_in1.fastq.gz") + " -cut 10" + " -out_index out/FastqExtractBarcode_out1.fastq.gz" + " -out_main out/FastqExtractBarcode_out2.fastq.gz");
		COMPARE_GZ_FILES("out/FastqExtractBarcode_out1.fastq.gz", TESTDATA("data_out/FastqExtractBarcode_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/FastqExtractBarcode_out2.fastq.gz", TESTDATA("data_out/FastqExtractBarcode_out2.fastq.gz"));
	}

	void test_02()
	{
		EXECUTE( "FastqExtractBarcode", "-in " + TESTDATA("data_in/FastqExtractBarcode_in2.fastq.gz") + " -cut 10" + " -out_index out/FastqExtractBarcode_out3.fastq.gz" + " -out_main out/FastqExtractBarcode_out4.fastq.gz");
		COMPARE_GZ_FILES("out/FastqExtractBarcode_out3.fastq.gz", TESTDATA("data_out/FastqExtractBarcode_out3.fastq.gz"));
		COMPARE_GZ_FILES("out/FastqExtractBarcode_out4.fastq.gz", TESTDATA("data_out/FastqExtractBarcode_out4.fastq.gz"));
	}

	void test_03()
	{
		EXECUTE( "FastqExtractBarcode", "-in " + TESTDATA("data_in/FastqExtractBarcode_in2.fastq.gz") + " -cut 10 -out_index out/FastqExtractBarcode_out3.fastq.gz" + " -out_main out/FastqExtractBarcode_out4.fastq.gz");
	}
};
