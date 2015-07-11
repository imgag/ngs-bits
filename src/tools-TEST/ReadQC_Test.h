#include "TestFramework.h"

TEST_CLASS(ReadQC_Test)
{
Q_OBJECT
private slots:

	void base_test()
	{
		TFW_EXEC("ReadQC", "-in1 " + QFINDTESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out out/ReadQC_out1.qcML");
		TFW::removeLinesContaining("out/ReadQC_out1.qcML", "creation ");
		TFW::removeLinesContaining("out/ReadQC_out1.qcML", "<binary>");
		TFW::comareFiles("out/ReadQC_out1.qcML", QFINDTESTDATA("data_out/ReadQC_out1.qcML"));
	}

	void with_txt_parameter()
	{
		TFW_EXEC("ReadQC", "-in1 " + QFINDTESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out out/ReadQC_out2.txt -txt");
		TFW::comareFiles("out/ReadQC_out2.txt", QFINDTESTDATA("data_out/ReadQC_out2.txt"));
	}

	void single_end()
	{
		TFW_EXEC("ReadQC", "-in1 " + QFINDTESTDATA("data_in/ReadQC_in1.fastq.gz") + " -out out/ReadQC_out3.qcML");
		TFW::removeLinesContaining("out/ReadQC_out3.qcML", "creation ");
		TFW::removeLinesContaining("out/ReadQC_out3.qcML", "<binary>");
		TFW::comareFiles("out/ReadQC_out3.qcML", QFINDTESTDATA("data_out/ReadQC_out3.qcML"));
	}
};

