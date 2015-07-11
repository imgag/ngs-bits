#include "TestFramework.h"

TEST_CLASS(BedExtend_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("BedExtend", "-in " + QFINDTESTDATA("data_in/BedExtend_in1.bed") + " -out out/BedExtend_test01_out.bed -n 100");
		TFW::comareFiles("out/BedExtend_test01_out.bed", QFINDTESTDATA("data_out/BedExtend_test01_out.bed"));
	}
	
	void test_02()
	{
		TFW_EXEC("BedExtend", "-in " + QFINDTESTDATA("data_in/BedExtend_in2.bed") + " -fai " + QFINDTESTDATA("data_in/hg19.fa.fai") + " -out out/BedExtend_test02_out.bed -n 100");
		TFW::comareFiles("out/BedExtend_test02_out.bed", QFINDTESTDATA("data_out/BedExtend_test02_out.bed"));
	}

};
