#include "TestFramework.h"


TEST_CLASS(BamDownsample_Test)
{
Q_OBJECT
private slots:

	void paired_end()
	{
		EXECUTE("BamDownsample", "-in " + TESTDATA("data_in/BamDownsample_in1.bam") + " -out out/BamDownsample_out1.bam -percentage 20 -test");
		IS_TRUE(QFile::exists("out/BamDownsample_out1.bam"));

        //The random number generator behaves differently under Linux/OSX/Windows => we need separate expected test results
		if (Helper::isWindows())
		{
			COMPARE_FILES("out/BamDownsample_Test_line11.log", TESTDATA("data_out/BamDownsample_out1_Windows.txt"));
		}
		else if (Helper::isMacOS())
		{
			COMPARE_FILES("out/BamDownSample_Test_line11.log", TESTDATA("data_out/BamDownSample_out1_OSX.txt"));
		}
		else
		{
			COMPARE_FILES("out/BamDownsample_Test_line11.log", TESTDATA("data_out/BamDownsample_out1_Linux.txt"));
		}
	}
};
