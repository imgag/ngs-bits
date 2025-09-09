#include "TestFramework.h"

TEST_CLASS(QcToTsv_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		EXECUTE("QcToTsv", "-in " + TESTDATA("data_in/QcToTsv_in1.qcML") + " " + TESTDATA("data_in/QcToTsv_in2.qcML") + " " + TESTDATA("data_in/QcToTsv_in3.qcML") + " " + TESTDATA("data_in/QcToTsv_in4.qcML") + " -out out/QcToTsv_out1.tsv");
		COMPARE_FILES("out/QcToTsv_out1.tsv", TESTDATA("data_out/QcToTsv_out1.tsv"));
	}
};

