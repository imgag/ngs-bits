#include "TestFramework.h"

TEST_CLASS(BedCoverage_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test01_out.tsv");
		COMPARE_FILES("out/BedCoverage_test01_out.tsv", TESTDATA("data_out/BedCoverage_test01_out.tsv"));
	}
	
	//optional parameter: anom
	void test_02()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test02_out.tsv -anom");
		COMPARE_FILES("out/BedCoverage_test02_out.tsv", TESTDATA("data_out/BedCoverage_test02_out.tsv"));
	}
	
	//optional parameters: anom, min_mapq
	void test_03()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test03_out.tsv -anom -min_mapq 0");
		COMPARE_FILES("out/BedCoverage_test03_out.tsv", TESTDATA("data_out/BedCoverage_test03_out.tsv"));
	}
	
	//two input files, optional parameters: anom
	void test_04()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test04_out.tsv -anom");
		COMPARE_FILES("out/BedCoverage_test04_out.tsv", TESTDATA("data_out/BedCoverage_test04_out.tsv"));
	}
};
