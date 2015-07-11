#include "TestFramework.h"

TEST_CLASS(FastqList_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("FastqList", "-in " + QFINDTESTDATA("data_in/FastqList_in1.fastq.gz") + " -out out/FastqList_out1.txt");
		TFW::comareFilesGZ("out/FastqList_out1.txt", QFINDTESTDATA("data_out/FastqList_out1.txt"));
	}

};
