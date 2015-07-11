#include "TestFramework.h"

TEST_CLASS(SampleDiff_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC_FAIL("SampleDiff", "-in1 " + QFINDTESTDATA("data_in/SampleDiff_in1.tsv") + " -in2 " + QFINDTESTDATA("data_in/SampleDiff_in2.tsv") + " -out out/SampleDiff_out1.txt");
		TFW::comareFiles("out/SampleDiff_out1.txt", QFINDTESTDATA("data_out/SampleDiff_out1.txt"));
	}
	
	void test_02()
	{
		TFW_EXEC_FAIL("SampleDiff", "-in1 " + QFINDTESTDATA("data_in/SampleDiff_in1.tsv") + " -in2 " + QFINDTESTDATA("data_in/SampleDiff_in2.tsv") + " -out out/SampleDiff_out2.txt -sm");
		TFW::comareFiles("out/SampleDiff_out2.txt", QFINDTESTDATA("data_out/SampleDiff_out2.txt"));
	}
	
	void test_03()
	{
		TFW_EXEC_FAIL("SampleDiff", "-in1 " + QFINDTESTDATA("data_in/SampleDiff_in1.tsv") + " -in2 " + QFINDTESTDATA("data_in/SampleDiff_in2.tsv") + " -out out/SampleDiff_out3.txt -ei");
		TFW::comareFiles("out/SampleDiff_out3.txt", QFINDTESTDATA("data_out/SampleDiff_out3.txt"));
	}

};

