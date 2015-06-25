#include "../TestFramework.h"

class FastqFormat_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("FastqFormat", "-in " + QFINDTESTDATA("data_in/FastqFormat_in1.fastq") + " -out out/FastqFormat_out1.txt");
		TFW::comareFiles("out/FastqFormat_out1.txt", QFINDTESTDATA("data_out/FastqFormat_out1.txt"));
	}
	
	void test_02()
	{
		TFW_EXEC("FastqFormat", "-in " + QFINDTESTDATA("data_in/FastqFormat_in2.fastq.gz") + " -out out/FastqFormat_out2.txt");
		TFW::comareFiles("out/FastqFormat_out2.txt", QFINDTESTDATA("data_out/FastqFormat_out2.txt"));
	}

};

TFW_DECLARE(FastqFormat_Test)


