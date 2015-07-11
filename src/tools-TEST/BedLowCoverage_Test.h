#include "TestFramework.h"

TEST_CLASS(BedLowCoverage_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("BedLowCoverage", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedLowCoverage_test01_out.bed -cutoff 20");
		TFW::comareFiles("out/BedLowCoverage_test01_out.bed", QFINDTESTDATA("data_out/BedLowCoverage_test01_out.bed"));
	}

};
