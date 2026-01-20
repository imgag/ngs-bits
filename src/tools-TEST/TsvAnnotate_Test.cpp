#include "TestFramework.h"

TEST_CLASS(TsvAnnotate_Test)
{
private:

	TEST_METHOD(with_all_parameters)
	{
		EXECUTE("TsvAnnotate", "-in1 " + TESTDATA("data_in/TsvAnnotate_in1.tsv") + " -c1 ps -in2 " + TESTDATA("data_in/TsvAnnotate_in2.tsv") + " -c2 ps2 -anno score2,score3 -mv XXX -out out/TsvAnnotate_out1.tsv");
		COMPARE_FILES("out/TsvAnnotate_out1.tsv", TESTDATA("data_out/TsvAnnotate_out1.tsv"));
	}


	TEST_METHOD(without_optional_parameter)
	{
		EXECUTE("TsvAnnotate", "-in1 " + TESTDATA("data_in/TsvAnnotate_in1.tsv") + " -c1 ps -in2 " + TESTDATA("data_in/TsvAnnotate_in1.tsv") + " -anno score -out out/TsvAnnotate_out2.tsv");
		COMPARE_FILES("out/TsvAnnotate_out1.tsv", TESTDATA("data_out/TsvAnnotate_out1.tsv"));
	}
};
