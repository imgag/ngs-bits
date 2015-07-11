#include "TestFramework.h"

TEST_CLASS(SampleCorrelation_Test)
{
Q_OBJECT
private slots:
	
	void test_tsv1()
	{
		TFW_EXEC("SampleCorrelation", "-in1 " + QFINDTESTDATA("data_in/SampleCorrelation_in1.tsv") + " -in2 " + QFINDTESTDATA("data_in/SampleCorrelation_in2.tsv") + " -out out/SampleCorrelation_out1.txt");
		TFW::comareFiles("out/SampleCorrelation_out1.txt", QFINDTESTDATA("data_out/SampleCorrelation_out1.txt"));
	}
	
	void test_tsv2()
	{
		TFW_EXEC("SampleCorrelation", "-in1 " + QFINDTESTDATA("data_in/SampleCorrelation_in1.tsv") + " -in2 " + QFINDTESTDATA("data_in/SampleCorrelation_in3.tsv") + " -out out/SampleCorrelation_out2.txt");
		TFW::comareFiles("out/SampleCorrelation_out2.txt", QFINDTESTDATA("data_out/SampleCorrelation_out2.txt"));
	}
	
	void test_bam1()
	{
		QString in1 = "/mnt/share/data/test_data/GS120240.bam";
		QString in2 = "/mnt/share/data/test_data/GS120180.bam";
#ifdef WIN32
		in1 = "W:\\share\\data\\test_data\\GS120240.bam";
		in2 = "W:\\share\\data\\test_data\\GS120180.bam";
		if (!QDir("W:\\share\\").exists()) QSKIP("Test needs data from W: drive!");
#endif
	
		TFW_EXEC("SampleCorrelation", "-in1 " + in1 + " -in2 " + in2 + " -out out/SampleCorrelation_out3.txt -bam");
		TFW::comareFiles("out/SampleCorrelation_out3.txt", QFINDTESTDATA("data_out/SampleCorrelation_out3.txt"));
	}
	
};
