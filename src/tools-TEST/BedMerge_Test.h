#include "../TestFramework.h"

class BedMerge_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("BedMerge", "-in " + QFINDTESTDATA("data_in/exome.bed") + " -out out/BedMerge_test01_out.bed");
		TFW::comareFiles("out/BedMerge_test01_out.bed", QFINDTESTDATA("data_out/BedMerge_test01_out.bed"));
	}
	
	void test_02()
	{
		TFW_EXEC("BedMerge", "-in " + QFINDTESTDATA("data_in/BedMerge_in1.bed") + " -out out/BedMerge_test02_out.bed");
		TFW::comareFiles("out/BedMerge_test02_out.bed", QFINDTESTDATA("data_out/BedMerge_test02_out.bed"));
	}

};

TFW_DECLARE(BedMerge_Test)

