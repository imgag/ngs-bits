#include "TestFramework.h"

TEST_CLASS(SampleCorrelation_Test)
{
Q_OBJECT
private slots:
	
	void test_tsv1()
	{
		EXECUTE("SampleCorrelation", "-in1 " + TESTDATA("data_in/SampleCorrelation_in1.tsv") + " -in2 " + TESTDATA("data_in/SampleCorrelation_in2.tsv") + " -out out/SampleCorrelation_out1.txt");
		COMPARE_FILES("out/SampleCorrelation_out1.txt", TESTDATA("data_out/SampleCorrelation_out1.txt"));
	}
	
	void test_tsv2()
	{
		EXECUTE("SampleCorrelation", "-in1 " + TESTDATA("data_in/SampleCorrelation_in1.tsv") + " -in2 " + TESTDATA("data_in/SampleCorrelation_in3.tsv") + " -out out/SampleCorrelation_out2.txt");
		COMPARE_FILES("out/SampleCorrelation_out2.txt", TESTDATA("data_out/SampleCorrelation_out2.txt"));
	}
	
	void test_bam1()
	{
		QString in1 = "/mnt/share/data/test_data/GS120240.bam";
		QString in2 = "/mnt/share/data/test_data/GS120180.bam";
#ifdef WIN32
		in1 = "W:\\share\\data\\test_data\\GS120240.bam";
		in2 = "W:\\share\\data\\test_data\\GS120180.bam";
		if (!QDir("W:\\share\\").exists()) SKIP("Test needs data from W: drive!");
#endif
	
		EXECUTE("SampleCorrelation", "-in1 " + in1 + " -in2 " + in2 + " -out out/SampleCorrelation_out3.txt -bam");
		COMPARE_FILES("out/SampleCorrelation_out3.txt", TESTDATA("data_out/SampleCorrelation_out3.txt"));
	}
	
};
