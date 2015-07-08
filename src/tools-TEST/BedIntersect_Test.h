#include "TestFramework.h"

class BedIntersect_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("BedIntersect", "-in1 " + QFINDTESTDATA("data_in/exome.bed") + " -in2 " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test01_out.bed");
		TFW::comareFiles("out/BedIntersect_test01_out.bed", QFINDTESTDATA("data_out/BedIntersect_test01_out.bed"));
	}
	
	void test_02()
	{
		TFW_EXEC("BedIntersect", "-in1 " + QFINDTESTDATA("data_in/exome.bed") + " -in2 " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test02_out.bed -mode in1");
		TFW::comareFiles("out/BedIntersect_test02_out.bed", QFINDTESTDATA("data_out/BedIntersect_test02_out.bed"));
	}
	
	void test_03()
	{
		TFW_EXEC("BedIntersect", "-in1 " + QFINDTESTDATA("data_in/exome.bed") + " -in2 " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test03_out.bed -mode in2");
		TFW::comareFiles("out/BedIntersect_test03_out.bed", QFINDTESTDATA("data_out/BedIntersect_test03_out.bed"));
	}

};

TFW_DECLARE(BedIntersect_Test)


