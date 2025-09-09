#include "TestFramework.h"


TEST_CLASS(BamInfo_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
        EXECUTE("BamInfo", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -name -out out/BamInfo_out1.tsv");
        COMPARE_FILES("out/BamInfo_out1.tsv", TESTDATA("data_out/BamInfo_out1.tsv"));
	}
	
};

