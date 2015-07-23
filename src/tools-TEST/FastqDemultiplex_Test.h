#include "TestFramework.h"

TEST_CLASS(FastqDemultiplex_Test)
{
Q_OBJECT
private slots:

	void test_default()
	{
		EXECUTE("FastqDemultiplex", "-in1 " +TESTDATA("data_in/FastqDemultiplex_in1_1.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in1_3.fastq") + " -in2 "+TESTDATA("data_in/FastqDemultiplex_in1_2.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in1_4.fastq")+ " -sheet " + TESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex1 -rev2 -summary FastqDemultiplex_out1.txt");
		COMPARE_FILES("out/FastqDemultiplex1/FastqDemultiplex_out1.txt", TESTDATA("data_out/FastqDemultiplex_out1.txt"));
    }

	//demultiplex one of the matched result files again to make sure no reads get lost during demultiplexing
	void test_default2()
    {
		EXECUTE("FastqDemultiplex", "-in1 out/FastqDemultiplex1/Project_CaMyo/Sample_GS150167_01/GS150167_01_TAAGGCGAGCGATCTA_L001_R1_001.fastq.gz -in2 out/FastqDemultiplex1/Project_CaMyo/Sample_GS150167_01/GS150167_01_TAAGGCGAGCGATCTA_L001_R2_001.fastq.gz -sheet " + TESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex2 -rev2 -summary FastqDemultiplex_out2.txt");
		COMPARE_FILES("out/FastqDemultiplex2/FastqDemultiplex_out2.txt", TESTDATA("data_out/FastqDemultiplex_out2.txt"));
    }

	void test_nomismatches()
	{
		EXECUTE("FastqDemultiplex", "-in1 " +TESTDATA("data_in/FastqDemultiplex_in1_1.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in1_3.fastq") + " -in2 "+TESTDATA("data_in/FastqDemultiplex_in1_2.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in1_4.fastq")+ " -sheet " + TESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex3 -mms 0 -mmd 0 -rev2 -summary FastqDemultiplex_out3.txt");
		COMPARE_FILES("out/FastqDemultiplex3/FastqDemultiplex_out3.txt", TESTDATA("data_out/FastqDemultiplex_out3.txt"));
    }

	//demultiplex the unassiged result files again to make sure no reads get lost during demultiplexing
	void test_nomismatches2()
    {
		EXECUTE("FastqDemultiplex", "-in1 " +TESTDATA("out/FastqDemultiplex3/unassigned_L001_R1.fastq.gz") + " -in2 "+TESTDATA("out/FastqDemultiplex3/unassigned_L001_R2.fastq.gz") + " -sheet " + TESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex4 -mms 0 -mmd 0 -rev2 -summary FastqDemultiplex_out4.txt");
		COMPARE_FILES("out/FastqDemultiplex4/FastqDemultiplex_out4.txt", TESTDATA("data_out/FastqDemultiplex_out4.txt"));
    }

	//demultiplexing allowing 4 mismatches in single index
	void test_many_mismatch()
	{
		EXECUTE("FastqDemultiplex", "-in1 " +TESTDATA("data_in/FastqDemultiplex_in1_1.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in1_3.fastq") + " -in2 "+TESTDATA("data_in/FastqDemultiplex_in1_2.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in1_4.fastq")+ " -sheet " + TESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex5 -rev2 -summary FastqDemultiplex_out5.txt -mms 4");
		COMPARE_FILES("out/FastqDemultiplex5/FastqDemultiplex_out5.txt", TESTDATA("data_out/FastqDemultiplex_out5.txt"));
	}

	//test multiple lanes, one file pair per lane
	void test_multiple_lanes()
	{
		EXECUTE("FastqDemultiplex", "-in1 " +TESTDATA("data_in/FastqDemultiplex_in1_1.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in2_1.fastq") + " -in2 "+TESTDATA("data_in/FastqDemultiplex_in1_2.fastq") + " " + TESTDATA("data_in/FastqDemultiplex_in2_2.fastq")+ " -sheet " + TESTDATA("data_in/FastqDemultiplex_in2.tsv") + " -out out/FastqDemultiplex6 -rev2 -summary FastqDemultiplex_out6.txt");
		COMPARE_FILES("out/FastqDemultiplex6/FastqDemultiplex_out6.txt", TESTDATA("data_out/FastqDemultiplex_out6.txt"));
	}

	//test multiple lanes, same data as above, but in only one file pair
	void test_multiple_lanes2()
	{
		EXECUTE("FastqDemultiplex", "-in1 " +TESTDATA("data_in/FastqDemultiplex_in3.fastq")+ " -in2 "+TESTDATA("data_in/FastqDemultiplex_in4.fastq")+ " -sheet " + TESTDATA("data_in/FastqDemultiplex_in2.tsv") + " -out out/FastqDemultiplex7 -rev2 -summary FastqDemultiplex_out7.txt");
		COMPARE_FILES("out/FastqDemultiplex7/FastqDemultiplex_out7.txt", TESTDATA("data_out/FastqDemultiplex_out6.txt"));
	}
};
