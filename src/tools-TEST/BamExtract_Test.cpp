#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(BamExtract_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("BamExtract", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -ids " + TESTDATA("data_in/BamExtract_ids.txt") + " -out out/BamExtract_out1.bam -out2 out/BamExtract_out2.bam");
		COMPARE_GZ_FILES("out/BamExtract_out1.bam", TESTDATA("data_out/BamExtract_out1.bam"));
		IS_TRUE(QFile::exists("out/BamExtract_out2.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamExtract_out1.log"));
	}
	
};

