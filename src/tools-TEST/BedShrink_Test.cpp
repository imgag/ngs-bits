#include "TestFramework.h"

TEST_CLASS(BedShrink_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("BedShrink", "-in " + TESTDATA("data_in/BedShrink_in1.bed") + " -out out/BedShrink_test01_out.bed -n 25");
		COMPARE_FILES("out/BedShrink_test01_out.bed", TESTDATA("data_out/BedShrink_test01_out.bed"));
	}
	
};
