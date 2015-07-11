#include "TestFramework.h"

TEST_CLASS(FastqConvert_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("FastqConvert", "-in " + QFINDTESTDATA("data_in/FastqConvert_in1.fastq.gz") + " -out out/FastqConvert_out1.fastq.gz");
		TFW::comareFilesGZ("out/FastqConvert_out1.fastq.gz", QFINDTESTDATA("data_out/FastqConvert_out1.fastq.gz"));
	}

};
