#include "TestFramework.h"

class BedIntersect_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void mode_intersect()
	{
		TFW_EXEC("BedIntersect", "-in " + QFINDTESTDATA("data_in/exome.bed") + " -in2 " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test01_out.bed");
		TFW::comareFiles("out/BedIntersect_test01_out.bed", QFINDTESTDATA("data_out/BedIntersect_test01_out.bed"));
	}
	
	void mode_in()
	{
		TFW_EXEC("BedIntersect", "-in " + QFINDTESTDATA("data_in/exome.bed") + " -in2 " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test02_out.bed -mode in");
		TFW::comareFiles("out/BedIntersect_test02_out.bed", QFINDTESTDATA("data_out/BedIntersect_test02_out.bed"));
	}
	
	void mode_in2()
	{
		TFW_EXEC("BedIntersect", "-in " + QFINDTESTDATA("data_in/exome.bed") + " -in2 " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test03_out.bed -mode in2");
		TFW::comareFiles("out/BedIntersect_test03_out.bed", QFINDTESTDATA("data_out/BedIntersect_test03_out.bed"));
	}

};

TFW_DECLARE(BedIntersect_Test)


