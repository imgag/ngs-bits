#include "TestFramework.h"

TEST_CLASS(TsvMerge_Test)
{
private:

	TEST_METHOD(by_number)
	{
		EXECUTE("TsvMerge", "-cols 1,2,3 -numeric -in " + TESTDATA("data_in/TsvMerge_in1.tsv") + " " + TESTDATA("data_in/TsvMerge_in2.tsv") + " -out out/TsvMerge_out1.tsv");
		COMPARE_FILES("out/TsvMerge_out1.tsv", TESTDATA("data_out/TsvMerge_out1.tsv"));
	}

	TEST_METHOD(by_name)
	{
		EXECUTE("TsvMerge", "-cols chr,start,end -mv missing -in " + TESTDATA("data_in/TsvMerge_in1.tsv") + " " + TESTDATA("data_in/TsvMerge_in2.tsv") + " " + TESTDATA("data_in/TsvMerge_in3.tsv") + " -out out/TsvMerge_out2.tsv");
		COMPARE_FILES("out/TsvMerge_out2.tsv", TESTDATA("data_out/TsvMerge_out2.tsv"));
	}

	TEST_METHOD(simple_by_name)
	{
		EXECUTE("TsvMerge", "-cols chr,start,end -simple -in " + TESTDATA("data_in/TsvMerge_in1.tsv") + " " + TESTDATA("data_in/TsvMerge_in4.tsv") + " " + TESTDATA("data_in/TsvMerge_in5.tsv") + " -out out/TsvMerge_out3.tsv");
		COMPARE_FILES("out/TsvMerge_out3.tsv", TESTDATA("data_out/TsvMerge_out3.tsv"));
	}
};


