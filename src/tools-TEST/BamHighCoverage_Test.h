#include "TestFramework.h"

TEST_CLASS(BamHighCoverage_Test)
{
Q_OBJECT
private slots:
	
	void cov20()
	{
		EXECUTE("BamHighCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BamHighCoverage_test01_out.bed -cutoff 20 -min_mapq 20");
		COMPARE_FILES("out/BamHighCoverage_test01_out.bed", TESTDATA("data_out/BamHighCoverage_test01_out.bed"));
	}

};
