#include "../TestFramework.h"

class FastqMidParser_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("FastqMidParser", "-in " + QFINDTESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out1.txt");
		TFW::comareFiles("out/FastqMidParser_out1.txt", QFINDTESTDATA("data_out/FastqMidParser_out1.txt"));
	}
	
	void test_02()
	{
		TFW_EXEC("FastqMidParser", "-in " + QFINDTESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out2.txt -lines 500 -mids 5");
		TFW::comareFiles("out/FastqMidParser_out2.txt", QFINDTESTDATA("data_out/FastqMidParser_out2.txt"));
	}
	
	void test_03()
	{
		TFW_EXEC("FastqMidParser", "-in " + QFINDTESTDATA("data_in/FastqMidParser_in1.fastq.gz") + " -out out/FastqMidParser_out3.txt -sheet " + QFINDTESTDATA("data_in/FastqMidParser_in1.csv"));
		TFW::comareFiles("out/FastqMidParser_out3.txt", QFINDTESTDATA("data_out/FastqMidParser_out3.txt"));
	}

};

TFW_DECLARE(FastqMidParser_Test)

