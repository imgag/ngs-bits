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
	
	void test_bam()
	{
		EXECUTE("SampleCorrelation", "-in1 " + TESTDATA("data_in/SampleCorrelation_in4.bam") + " -in2 " + TESTDATA("data_in/SampleCorrelation_in5.bam") + " -out out/SampleCorrelation_out3.txt -bam -max_snps 200");
		COMPARE_FILES("out/SampleCorrelation_out3.txt", TESTDATA("data_out/SampleCorrelation_out3.txt"));
	}

	void test_bam_roi()
	{
		EXECUTE("SampleCorrelation", "-in1 " + TESTDATA("data_in/SampleCorrelation_in4.bam") + " -in2 " + TESTDATA("data_in/SampleCorrelation_in5.bam") + " -out out/SampleCorrelation_out4.txt -bam -max_snps 100 -roi " + TESTDATA("data_in/SampleCorrelation_roi.bed"));
		COMPARE_FILES("out/SampleCorrelation_out4.txt", TESTDATA("data_out/SampleCorrelation_out4.txt"));
	}

};
