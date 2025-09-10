#include "TestFramework.h"

TEST_CLASS(BedChunk_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("BedChunk", "-in " + TESTDATA("data_in/BedChunk_in1.bed") + " -out out/BedChunk_out1.bed -n 100");
		COMPARE_FILES("out/BedChunk_out1.bed", TESTDATA("data_out/BedChunk_out1.bed"));
	}
};
