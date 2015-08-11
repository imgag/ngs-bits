#include "TestFramework.h"


TEST_CLASS(BamCleanHaloplex_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		EXECUTE("BamCleanHaloplex", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BamCleanHaloplex_out1.bam");
		IS_TRUE(QFile::exists("out/BamCleanHaloplex_out1.bam"));
		COMPARE_FILES("out/BamCleanHaloplex_Test_line11.log", TESTDATA("data_out/BamCleanHaloplex_out1.log"));
	}
	
};

