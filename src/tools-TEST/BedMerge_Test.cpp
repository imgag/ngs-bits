#include "TestFramework.h"

TEST_CLASS(BedMerge_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("BedMerge", "-in " + TESTDATA("data_in/exome.bed") + " -out out/BedMerge_test01_out.bed");
		COMPARE_FILES("out/BedMerge_test01_out.bed", TESTDATA("data_out/BedMerge_test01_out.bed"));
	}
	
	TEST_METHOD(test_02)
	{
		EXECUTE("BedMerge", "-in " + TESTDATA("data_in/BedMerge_in1.bed") + " -out out/BedMerge_test02_out.bed");
		COMPARE_FILES("out/BedMerge_test02_out.bed", TESTDATA("data_out/BedMerge_test02_out.bed"));
	}

};
