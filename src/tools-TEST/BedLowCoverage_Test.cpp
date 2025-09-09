#include "TestFramework.h"

TEST_CLASS(BedLowCoverage_Test)
{
private:
	
	TEST_METHOD(sweep)
	{
		EXECUTE("BedLowCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedLowCoverage_test01_out.bed -cutoff 20");
		COMPARE_FILES("out/BedLowCoverage_test01_out.bed", TESTDATA("data_out/BedLowCoverage_test01_out.bed"));
	}

	TEST_METHOD(random_access)
	{
		EXECUTE("BedLowCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedLowCoverage_test02_out.bed -cutoff 20 -random_access");
		COMPARE_FILES("out/BedLowCoverage_test02_out.bed", TESTDATA("data_out/BedLowCoverage_test01_out.bed"));
	}

	TEST_METHOD(sweep_mq20_bq30)
	{
		EXECUTE("BedLowCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedLowCoverage_test03_out.bed -cutoff 20 -min_mapq 20 -min_baseq 30");
		COMPARE_FILES("out/BedLowCoverage_test03_out.bed", TESTDATA("data_out/BedLowCoverage_test03_out.bed"));
	}

	TEST_METHOD(random_access_mq20_bq30)
	{
		EXECUTE("BedLowCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedLowCoverage_test04_out.bed -cutoff 20 -min_mapq 20 -min_baseq 30 -random_access");
		COMPARE_FILES("out/BedLowCoverage_test04_out.bed", TESTDATA("data_out/BedLowCoverage_test03_out.bed"));
	}

};
