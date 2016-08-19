#include "TestFramework.h"

TEST_CLASS(ReadQC_Test)
{
Q_OBJECT
private slots:

	void base_test()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out out/ReadQC_out1.qcML");
		REMOVE_LINES("out/ReadQC_out1.qcML", QRegExp("creation "));
		REMOVE_LINES("out/ReadQC_out1.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/ReadQC_out1.qcML", TESTDATA("data_out/ReadQC_out1.qcML"));
	}

	void with_txt_parameter()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out out/ReadQC_out2.txt -txt");
		COMPARE_FILES("out/ReadQC_out2.txt", TESTDATA("data_out/ReadQC_out2.txt"));
	}

	void single_end()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in1.fastq.gz") + " -out out/ReadQC_out3.qcML");
		REMOVE_LINES("out/ReadQC_out3.qcML", QRegExp("creation "));
		REMOVE_LINES("out/ReadQC_out3.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/ReadQC_out3.qcML", TESTDATA("data_out/ReadQC_out3.qcML"));
	}

	void different_read_lengths()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in3.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in4.fastq.gz") + " -out out/ReadQC_out4.qcML");
		REMOVE_LINES("out/ReadQC_out4.qcML", QRegExp("creation "));
		REMOVE_LINES("out/ReadQC_out4.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/ReadQC_out4.qcML", TESTDATA("data_out/ReadQC_out4.qcML"));
	}

	void multiple_input_files()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in1.fastq.gz") + " " + TESTDATA("data_in/ReadQC_in3.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in2.fastq.gz") + " " + TESTDATA("data_in/ReadQC_in4.fastq.gz") + " -out out/ReadQC_out5.qcML");
		REMOVE_LINES("out/ReadQC_out5.qcML", QRegExp("creation "));
		REMOVE_LINES("out/ReadQC_out5.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/ReadQC_out5.qcML", TESTDATA("data_out/ReadQC_out5.qcML"));
	}
};

