#include "TestFramework.h"

TEST_CLASS(FastqExtract_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("FastqExtract", "-in " + QFINDTESTDATA("data_in/FastqExtract_in1.fastq.gz") + " -ids " + QFINDTESTDATA("data_in/FastqExtract_in1.txt") + " -out out/FastqExtract_out1.fastq.gz");
		TFW::comareFilesGZ("out/FastqExtract_out1.fastq.gz", QFINDTESTDATA("data_out/FastqExtract_out1.fastq.gz"));
	}

	void test_02()
	{
		TFW_EXEC("FastqExtract", "-in " + QFINDTESTDATA("data_in/FastqExtract_in2.fastq.gz") + " -ids " + QFINDTESTDATA("data_in/FastqExtract_in2.txt") + " -out out/FastqExtract_out2.fastq.gz");
		TFW::comareFilesGZ("out/FastqExtract_out2.fastq.gz", QFINDTESTDATA("data_out/FastqExtract_out2.fastq.gz"));
	}
};
