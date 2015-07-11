#include "TestFramework.h"

class BedInfo_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void base_test()
	{
		TFW_EXEC("BedInfo", "-in " + QFINDTESTDATA("data_in/exome.bed") + " -out out/BedInfo_test01_out.txt");
		TFW::comareFiles("out/BedInfo_test01_out.txt", QFINDTESTDATA("data_out/BedInfo_test01_out.txt"));
	}
	
	void with_parameter_fai()
	{
		TFW_EXEC("BedInfo", "-in " + QFINDTESTDATA("data_in/BedInfo_in2.bed") + " -out out/BedInfo_test02_out.txt -nomerge -fai " + QFINDTESTDATA("data_in/hg19.fa.fai"));
		TFW::comareFiles("out/BedInfo_test02_out.txt", QFINDTESTDATA("data_out/BedInfo_test02_out.txt"));
	}

	void with_parameter_filename()
	{
		TFW_EXEC("BedInfo", "-in " + QFINDTESTDATA("data_in/BedInfo_in2.bed") + " -out out/BedInfo_test03_out.txt -filename");
		TFW::comareFiles("out/BedInfo_test03_out.txt", QFINDTESTDATA("data_out/BedInfo_test03_out.txt"));
	}

};

TFW_DECLARE(BedInfo_Test)

