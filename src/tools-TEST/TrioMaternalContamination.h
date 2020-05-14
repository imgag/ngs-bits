#include "TestFramework.h"

TEST_CLASS(TrioMaternalContamination_Test)
{
Q_OBJECT
private slots:
	
	void trio()
	{
		//EXECUTE("TrioMaternalContamination", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedLowCoverage_test01_out.bed -cutoff 20");
		//COMPARE_FILES("out/BedLowCoverage_test01_out.bed", TESTDATA("data_out/BedLowCoverage_test01_out.bed"));
	}

};
