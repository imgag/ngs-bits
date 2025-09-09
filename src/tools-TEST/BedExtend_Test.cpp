#include "TestFramework.h"

TEST_CLASS(BedExtend_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("BedExtend", "-in " + TESTDATA("data_in/BedExtend_in1.bed") + " -out out/BedExtend_test01_out.bed -n 100");
		COMPARE_FILES("out/BedExtend_test01_out.bed", TESTDATA("data_out/BedExtend_test01_out.bed"));
	}
	
	TEST_METHOD(test_02)
	{
		EXECUTE("BedExtend", "-in " + TESTDATA("data_in/BedExtend_in2.bed") + " -fai " + TESTDATA("data_in/hg19.fa.fai") + " -out out/BedExtend_test02_out.bed -n 100");
		COMPARE_FILES("out/BedExtend_test02_out.bed", TESTDATA("data_out/BedExtend_test02_out.bed"));
	}

};
