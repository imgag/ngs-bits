#include "../TestFramework.h"

class VcfToTsv_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("VcfToTsv", "-in " + QFINDTESTDATA("data_in/VcfToTsv_in1.vcf") + " -out out/VcfToTsv_out1.tsv");
		TFW::comareFiles("out/VcfToTsv_out1.tsv", QFINDTESTDATA("data_out/VcfToTsv_out1.tsv"));
	}
	
	void test_02()
	{
		TFW_EXEC("VcfToTsv", "-in " + QFINDTESTDATA("data_in/VcfToTsv_in2.vcf") + " -out out/VcfToTsv_out2.tsv -split");
		TFW::comareFiles("out/VcfToTsv_out2.tsv", QFINDTESTDATA("data_out/VcfToTsv_out2.tsv"));
	}

};

TFW_DECLARE(VcfToTsv_Test)

