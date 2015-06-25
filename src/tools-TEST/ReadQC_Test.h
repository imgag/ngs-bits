#include "../TestFramework.h"

class ReadQC_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("ReadQC", "-in1 " + QFINDTESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out out/ReadQC_out1.txt -txt");
		TFW::comareFiles("out/ReadQC_out1.txt", QFINDTESTDATA("data_out/ReadQC_out1.txt"));
	}
	
	void test_02()
	{
		TFW_EXEC("ReadQC", "-in1 " + QFINDTESTDATA("data_in/ReadQC_in1.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/ReadQC_in2.fastq.gz") + " -out out/ReadQC_out2.qcML");
		TFW::removeLinesContaining("out/ReadQC_out2.qcML", "creation ");
		TFW::removeLinesContaining("out/ReadQC_out2.qcML", "<binary>");
		TFW::comareFiles("out/ReadQC_out2.qcML", QFINDTESTDATA("data_out/ReadQC_out2.qcML"));
	}

};

TFW_DECLARE(ReadQC_Test)


