#include "TestFramework.h"

TEST_CLASS(FastqTrim_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("FastqTrim", "-in " + QFINDTESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out1.fastq.gz -start 5");
		TFW::comareFilesGZ("out/FastqTrim_out1.fastq.gz", QFINDTESTDATA("data_out/FastqTrim_out1.fastq.gz"));
	}
	
	void test_02()
	{
		TFW_EXEC("FastqTrim", "-in " + QFINDTESTDATA("data_in/FastqTrim_in1.fastq.gz") + " -out out/FastqTrim_out2.fastq.gz -start 5 -end 5");
		TFW::comareFilesGZ("out/FastqTrim_out2.fastq.gz", QFINDTESTDATA("data_out/FastqTrim_out2.fastq.gz"));
	}

};
