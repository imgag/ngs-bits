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

	void multiple_threads()
	{
		for (int i=1; i<=8; ++i)
		{
			QTime timer;
			timer.start();
			EXECUTE("BedCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -threads " + QString::number(i) + " -out out/BedCoverage_test02_out.tsv");
			COMPARE_FILES_DELTA("out/BedCoverage_test02_out.tsv", TESTDATA("data_out/BedCoverage_test02_out.tsv"), 1.0, true, '\t'); //delta because of macOS rounding problem
			qDebug() << i << Helper::elapsedTime(timer); //TODO remove
		}
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
