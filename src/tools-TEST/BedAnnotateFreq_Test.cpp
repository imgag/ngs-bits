#include "TestFramework.h"

TEST_CLASS(BedAnnotateFreq_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("BedAnnotateFreq", "-in " + TESTDATA("data_in/BedAnnotateFreq_in1.bed") + " -bam " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedAnnotateFreq_test01_out.tsv");
		COMPARE_FILES("out/BedAnnotateFreq_test01_out.tsv", TESTDATA("data_out/BedAnnotateFreq_test01_out.tsv"));
	}
	
};
