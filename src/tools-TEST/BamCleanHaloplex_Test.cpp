#include "TestFramework.h"


TEST_CLASS(BamCleanHaloplex_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		EXECUTE("BamCleanHaloplex", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BamCleanHaloplex_out1.bam");
		IS_TRUE(QFile::exists("out/BamCleanHaloplex_out1.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamCleanHaloplex_out1.log"));
	}
	
};

