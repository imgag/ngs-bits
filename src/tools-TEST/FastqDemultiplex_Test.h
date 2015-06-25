#include "../TestFramework.h"

class FastqDemultiplex_Test
        : public QObject
{
    Q_OBJECT

private slots:

	void test_default()
	{
		TFW_EXEC("FastqDemultiplex", "-in1 " +QFINDTESTDATA("data_in/FastqDemultiplex_in1_1.fastq") + " " + QFINDTESTDATA("data_in/FastqDemultiplex_in1_3.fastq") + " -in2 "+QFINDTESTDATA("data_in/FastqDemultiplex_in1_2.fastq") + " " + QFINDTESTDATA("data_in/FastqDemultiplex_in1_4.fastq")+ " -sheet " + QFINDTESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex1 -rev2 -summary FastqDemultiplex_out1.txt");
		TFW::comareFiles("out/FastqDemultiplex1/FastqDemultiplex_out1.txt", QFINDTESTDATA("data_out/FastqDemultiplex_out1.txt"));
    }

	//demultiplex one of the matched result files again to make sure no reads get lost during demultiplexing
	void test_default2()
    {
		TFW_EXEC("FastqDemultiplex", "-in1 out/FastqDemultiplex1/Project_CaMyo/Sample_GS150167_01/GS150167_01_TAAGGCGAGCGATCTA_L001_R1_001.fastq.gz -in2 out/FastqDemultiplex1/Project_CaMyo/Sample_GS150167_01/GS150167_01_TAAGGCGAGCGATCTA_L001_R2_001.fastq.gz -sheet " + QFINDTESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex2 -rev2 -summary FastqDemultiplex_out2.txt");
		TFW::comareFiles("out/FastqDemultiplex2/FastqDemultiplex_out2.txt", QFINDTESTDATA("data_out/FastqDemultiplex_out2.txt"));
    }

	void test_nomismatches()
	{
		TFW_EXEC("FastqDemultiplex", "-in1 " +QFINDTESTDATA("data_in/FastqDemultiplex_in1_1.fastq") + " " + QFINDTESTDATA("data_in/FastqDemultiplex_in1_3.fastq") + " -in2 "+QFINDTESTDATA("data_in/FastqDemultiplex_in1_2.fastq") + " " + QFINDTESTDATA("data_in/FastqDemultiplex_in1_4.fastq")+ " -sheet " + QFINDTESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex3 -mms 0 -mmd 0 -rev2 -summary FastqDemultiplex_out3.txt");
		TFW::comareFiles("out/FastqDemultiplex3/FastqDemultiplex_out3.txt", QFINDTESTDATA("data_out/FastqDemultiplex_out3.txt"));
    }

	//demultiplex the unassiged result files again to make sure no reads get lost during demultiplexing
	void test_nomismatches2()
    {
		TFW_EXEC("FastqDemultiplex", "-in1 " +QFINDTESTDATA("out/FastqDemultiplex3/unassigned_L001_R1.fastq.gz") + " -in2 "+QFINDTESTDATA("out/FastqDemultiplex3/unassigned_L001_R2.fastq.gz") + " -sheet " + QFINDTESTDATA("data_in/FastqDemultiplex_in1.tsv") + " -out out/FastqDemultiplex4 -mms 0 -mmd 0 -rev2 -summary FastqDemultiplex_out4.txt");
		TFW::comareFiles("out/FastqDemultiplex4/FastqDemultiplex_out4.txt", QFINDTESTDATA("data_out/FastqDemultiplex_out4.txt"));
    }
};

TFW_DECLARE(FastqDemultiplex_Test)

