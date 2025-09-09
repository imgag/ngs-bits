#include "TestFramework.h"


TEST_CLASS(BamDownsample_Test)
{
private:

	TEST_METHOD(paired_end)
	{
		EXECUTE("BamDownsample", "-in " + TESTDATA("data_in/BamDownsample_in1.bam") + " -out out/BamDownsample_out1.bam -percentage 20 -test");
		IS_TRUE(QFile::exists("out/BamDownsample_out1.bam"));

        //The random number generator behaves differently under Linux/OSX/Windows => we need separate expected test results
		QString expected_file = TESTDATA("data_out/BamDownsample_out1_Linux.txt");
		if (Helper::isWindows()) expected_file =  TESTDATA("data_out/BamDownsample_out1_Windows.txt");
		else if (Helper::isMacOS()) expected_file =  TESTDATA("data_out/BamDownSample_out1_OSX.txt");
		COMPARE_FILES(lastLogFile(), expected_file);
	}
};
