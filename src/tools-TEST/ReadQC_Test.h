#include "TestFramework.h"

TEST_CLASS(ReadQC_Test)
{
Q_OBJECT
private slots:

	void base_test()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out out/ReadQC_out1.qcML");
        REMOVE_LINES("out/ReadQC_out1.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/ReadQC_out1.qcML", QRegularExpression("<binary>"));
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
        REMOVE_LINES("out/ReadQC_out3.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/ReadQC_out3.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/ReadQC_out3.qcML", TESTDATA("data_out/ReadQC_out3.qcML"));
	}

	void different_read_lengths()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in3.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in4.fastq.gz") + " -out out/ReadQC_out4.qcML");
        REMOVE_LINES("out/ReadQC_out4.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/ReadQC_out4.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES_DELTA("out/ReadQC_out4.qcML", TESTDATA("data_out/ReadQC_out4.qcML"), 0.01, false, '"'); //absolute delta because of rounding differences between Linux/Windows
	}

	void multiple_input_files()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in1.fastq.gz") + " " + TESTDATA("data_in/ReadQC_in3.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in2.fastq.gz") + " " + TESTDATA("data_in/ReadQC_in4.fastq.gz") + " -out out/ReadQC_out5.qcML");
        REMOVE_LINES("out/ReadQC_out5.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/ReadQC_out5.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/ReadQC_out5.qcML", TESTDATA("data_out/ReadQC_out5.qcML"));
	}


	void with_fastq_output()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out1 out/ReadQC_out6_R1.fastq.gz -out2 out/ReadQC_out6_R2.fastq.gz -out out/ReadQC_out6.qcML");
		COMPARE_GZ_FILES("out/ReadQC_out6_R1.fastq.gz", TESTDATA("data_in/ReadQC_in1.fastq.gz"));
		COMPARE_GZ_FILES("out/ReadQC_out6_R2.fastq.gz", TESTDATA("data_in/ReadQC_in2.fastq.gz"));
	}

	void long_read_test()
	{
		EXECUTE("ReadQC", "-in1 " + TESTDATA("data_in/ReadQC_in5.fastq.gz") + " -long_read -out out/ReadQC_out7.qcML");
        REMOVE_LINES("out/ReadQC_out7.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/ReadQC_out7.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/ReadQC_out7.qcML", TESTDATA("data_out/ReadQC_out7.qcML"));
	}
};

