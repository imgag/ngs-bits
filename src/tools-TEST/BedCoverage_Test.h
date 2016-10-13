#include "TestFramework.h"

TEST_CLASS(BedCoverage_Test)
{
Q_OBJECT
private slots:

	void default_parameters()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test01_out.tsv");
		COMPARE_FILES("out/BedCoverage_test01_out.tsv", TESTDATA("data_out/BedCoverage_test01_out.tsv"));
	}
	
	void min_mapq0_panelMode()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test03_out.tsv -min_mapq 0 -mode panel");
		COMPARE_FILES("out/BedCoverage_test03_out.tsv", TESTDATA("data_out/BedCoverage_test03_out.tsv"));
	}
	
	void two_input_files()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test04_out.tsv");
		COMPARE_FILES("out/BedCoverage_test04_out.tsv", TESTDATA("data_out/BedCoverage_test04_out.tsv"));
	}
};
