#include "TestFramework.h"

TEST_CLASS(BedReadCount_Test)
{
private:

	TEST_METHOD(default_parameters)
	{
		EXECUTE("BedReadCount", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedReadCount_test01_out.tsv");
		COMPARE_FILES("out/BedReadCount_test01_out.tsv", TESTDATA("data_out/BedReadCount_test01_out.tsv"));
	}

	TEST_METHOD(min_mapq0)
	{
		EXECUTE("BedReadCount", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedReadCount_test02_out.tsv -min_mapq 0");
		COMPARE_FILES("out/BedReadCount_test02_out.tsv", TESTDATA("data_out/BedReadCount_test02_out.tsv"));
	}
};
