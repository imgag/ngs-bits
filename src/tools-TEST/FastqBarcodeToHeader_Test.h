#include "TestFramework.h"

TEST_CLASS(FastqBarcodeToHeader_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		EXECUTE("FastqBarcodeToHeader", "-in " + TESTDATA("data_in/FastqBarcodeToHeader_in1.fastq.gz") + " -cut 10" + " -out_index out/FastqBarcodeToHeader_out1.fastq.gz" + " -out_main out/FastqBarcodeToHeader_out2.fastq.gz");
		COMPARE_GZ_FILES("out/FastqBarcodeToHeader_out1.fastq.gz", TESTDATA("data_out/FastqBarcodeToHeader_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/FastqBarcodeToHeader_out2.fastq.gz", TESTDATA("data_out/FastqBarcodeToHeader_out2.fastq.gz"));
	}

	void test_02()
	{
		EXECUTE("FastqBarcodeToHeader", "-in " + TESTDATA("data_in/FastqBarcodeToHeader_in2.fastq.gz") + " -cut 10" + " -out_index out/FastqBarcodeToHeader_out3.fastq.gz" + " -out_main out/FastqBarcodeToHeader_out4.fastq.gz");
		COMPARE_GZ_FILES("out/FastqBarcodeToHeader_out3.fastq.gz", TESTDATA("data_out/FastqBarcodeToHeader_out3.fastq.gz"));
		COMPARE_GZ_FILES("out/FastqBarcodeToHeader_out4.fastq.gz", TESTDATA("data_out/FastqBarcodeToHeader_out4.fastq.gz"));
	}

	void test_03()
	{
		EXECUTE("FastqBarcodeToHeader", "-in " + TESTDATA("data_in/FastqBarcodeToHeader_in2.fastq.gz") + " -cut 10 -mip_file " + TESTDATA("data_in/FastqBarcodeToHeader_in_mips1.txt") + " -out_index out/FastqBarcodeToHeader_out3.fastq.gz" + " -out_main out/FastqBarcodeToHeader_out4.fastq.gz");
		//TODO: Test after implementing cut to minimal mip
	}
};
