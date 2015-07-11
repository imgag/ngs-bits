#include "TestFramework.h"

TEST_CLASS(BedCoverage_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		TFW_EXEC("BedCoverage", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test01_out.tsv");
		TFW::comareFiles("out/BedCoverage_test01_out.tsv", QFINDTESTDATA("data_out/BedCoverage_test01_out.tsv"));
	}
	
	//optional parameter: anom
	void test_02()
	{
		TFW_EXEC("BedCoverage", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test02_out.tsv -anom");
		TFW::comareFiles("out/BedCoverage_test02_out.tsv", QFINDTESTDATA("data_out/BedCoverage_test02_out.tsv"));
	}
	
	//optional parameters: anom, min_mapq
	void test_03()
	{
		TFW_EXEC("BedCoverage", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test03_out.tsv -anom -min_mapq 0");
		TFW::comareFiles("out/BedCoverage_test03_out.tsv", QFINDTESTDATA("data_out/BedCoverage_test03_out.tsv"));
	}
	
	//two input files, optional parameters: anom
	void test_04()
	{
		TFW_EXEC("BedCoverage", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test04_out.tsv -anom");
		TFW::comareFiles("out/BedCoverage_test04_out.tsv", QFINDTESTDATA("data_out/BedCoverage_test04_out.tsv"));
	}
};
