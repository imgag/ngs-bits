#include "TestFramework.h"

class FastaInfo_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("FastaInfo", "-in " + QFINDTESTDATA("data_in/dummy.fa") + " -out out/FastaInfo_test01_out.txt");
		TFW::comareFiles("out/FastaInfo_test01_out.txt", QFINDTESTDATA("data_out/FastaInfo_test01_out.txt"));
	}

};

TFW_DECLARE(FastaInfo_Test)

