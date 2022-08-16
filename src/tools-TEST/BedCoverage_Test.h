#include "TestFramework.h"

TEST_CLASS(BedCoverage_Test)
{
Q_OBJECT
private slots:

	void default_parameters()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test01_out.tsv");
		COMPARE_FILES_DELTA("out/BedCoverage_test01_out.tsv", TESTDATA("data_out/BedCoverage_test01_out.tsv"), 1.0, true, '\t'); //delta because of macOS rounding problem
	}

	void with_duplicates()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("data_in/BedCoverage_in2.bed") + " -dup -bam " + TESTDATA("data_in/BedCoverage_in2.bam") + " -out out/BedCoverage_test02_out.tsv");
		COMPARE_FILES_DELTA("out/BedCoverage_test02_out.tsv", TESTDATA("data_out/BedCoverage_test02_out.tsv"), 1.0, true, '\t'); //delta because of macOS rounding problem
	}

	void min_mapq0_1decimal_clear()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test03_out.tsv -min_mapq 0 -decimals 1 -clear");
		COMPARE_FILES_DELTA("out/BedCoverage_test03_out.tsv", TESTDATA("data_out/BedCoverage_test03_out.tsv"), 1.0, true, '\t'); //delta because of macOS rounding problem
	}
	
	void two_input_files()
	{
		EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedCoverage_test04_out.tsv");
		COMPARE_FILES_DELTA("out/BedCoverage_test04_out.tsv", TESTDATA("data_out/BedCoverage_test04_out.tsv"), 1.0, true, '\t'); //delta because of macOS rounding problem
	}
};
