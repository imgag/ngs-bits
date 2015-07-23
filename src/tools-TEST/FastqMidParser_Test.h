#include "TestFramework.h"

TEST_CLASS(FastqMidParser_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("FastqMidParser", "-in " + TESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out1.txt");
		COMPARE_FILES("out/FastqMidParser_out1.txt", TESTDATA("data_out/FastqMidParser_out1.txt"));
	}
	
	void test_02()
	{
		EXECUTE("FastqMidParser", "-in " + TESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out2.txt -lines 500 -mids 5");
		COMPARE_FILES("out/FastqMidParser_out2.txt", TESTDATA("data_out/FastqMidParser_out2.txt"));
	}
	
	void test_03()
	{
		EXECUTE("FastqMidParser", "-in " + TESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out3.txt -sheet " + TESTDATA("data_in/FastqMidParser_in1.csv"));
		COMPARE_FILES("out/FastqMidParser_out3.txt", TESTDATA("data_out/FastqMidParser_out3.txt"));
	}

};
