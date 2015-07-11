#include "TestFramework.h"

TEST_CLASS(TsvInfo_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("TsvInfo", "-in " + QFINDTESTDATA("data_in/TsvInfo_in1.tsv") + " -out out/TsvInfo_out1.txt");
		TFW::comareFiles("out/TsvInfo_out1.txt", QFINDTESTDATA("data_out/TsvInfo_out1.txt"));
	}

	void test_02()
	{
		TFW_EXEC("TsvInfo", "-in " + QFINDTESTDATA("data_in/TsvInfo_in2.tsv") + " -out out/TsvInfo_out2.txt");
		TFW::comareFiles("out/TsvInfo_out2.txt", QFINDTESTDATA("data_out/TsvInfo_out2.txt"));
	}
};

