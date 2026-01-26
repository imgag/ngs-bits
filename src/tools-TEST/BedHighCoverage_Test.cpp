#include "TestFramework.h"

TEST_CLASS(BedHighCoverage_Test)
{
private:

	TEST_METHOD(sweep_mq20)
	{
		EXECUTE("BedHighCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedHighCoverage_test01_out.bed -cutoff 20");
		COMPARE_FILES("out/BedHighCoverage_test01_out.bed", TESTDATA("data_out/BedHighCoverage_test01_out.bed"));
	}

	TEST_METHOD(random_access)
	{
		EXECUTE("BedHighCoverage", + "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedHighCoverage_test02_out.bed -cutoff 20 -random_access");
		COMPARE_FILES("out/BedHighCoverage_test02_out.bed", TESTDATA("data_out/BedHighCoverage_test01_out.bed"));
	}

	TEST_METHOD(sweep_mq20_bq30)
	{
		EXECUTE("BedHighCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedHighCoverage_test03_out.bed -cutoff 20 -min_mapq 20 -min_baseq 30");
		COMPARE_FILES("out/BedHighCoverage_test03_out.bed", TESTDATA("data_out/BedHighCoverage_test03_out.bed"));
	}

	TEST_METHOD(random_access_mq20_bq30)
	{
		EXECUTE("BedHighCoverage", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedHighCoverage_test04_out.bed -cutoff 20 -min_mapq 20 -min_baseq 30 -random_access");
		COMPARE_FILES("out/BedHighCoverage_test04_out.bed", TESTDATA("data_out/BedHighCoverage_test03_out.bed"));
	}

};
