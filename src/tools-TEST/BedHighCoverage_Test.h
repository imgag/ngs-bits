#include "TestFramework.h"

TEST_CLASS(BedHighCoverage_Test)
{
Q_OBJECT
private slots:

	void roi_cov()
	{
		EXECUTE("BedHighCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedHighCoverage_test01_out.bed -cutoff 20");
		COMPARE_FILES("out/BedHighCoverage_test01_out.bed", TESTDATA("data_out/BedHighCoverage_test01_out.bed"));
	}
	
	void wgs_cov20()
	{
		EXECUTE("BedHighCoverage", + "-wgs -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedHighCoverage_test02_out.bed -cutoff 20 -min_mapq 20");
		COMPARE_FILES("out/BedHighCoverage_test02_out.bed", TESTDATA("data_out/BedHighCoverage_test02_out.bed"));
	}

};
