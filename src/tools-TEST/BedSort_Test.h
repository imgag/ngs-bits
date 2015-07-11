#include "TestFramework.h"

TEST_CLASS(BedSort_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("BedSort", "-in " + QFINDTESTDATA("data_in/exome.bed") + " -out out/BedSort_test01_out.bed");
		TFW::comareFiles("out/BedSort_test01_out.bed", QFINDTESTDATA("data_out/BedSort_test01_out.bed"));
	}
	
	void test_02()
	{
		TFW_EXEC("BedSort", "-in " + QFINDTESTDATA("data_in/BedSort_in2.bed") + " -out out/BedSort_test02_out.bed -uniq");
		TFW::comareFiles("out/BedSort_test02_out.bed", QFINDTESTDATA("data_out/BedSort_test02_out.bed"));
	}

};
