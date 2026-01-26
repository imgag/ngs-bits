#include "TestFramework.h"


TEST_CLASS(FastqDownsample_Test)
{
private:

	TEST_METHOD(paired_end)
	{
		EXECUTE("FastqDownsample", "-in1 " + TESTDATA("data_in/FastqDownsample_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/FastqDownsample_in2.fastq.gz") + " -out1 out/FastqDownsample_out1.fastq.gz -out2 out/FastqDownsample_out2.fastq.gz -percentage 20 -test");
		IS_TRUE(QFile::exists("out/FastqDownsample_out1.fastq.gz"));
		IS_TRUE(QFile::exists("out/FastqDownsample_out2.fastq.gz"));

        //The random number generator behaves differently under Linux/OSX/Windows => we need separate expected test results
		QString expected_file = TESTDATA("data_out/FastqDownsample_out1_Linux.txt");
		if (Helper::isWindows()) expected_file = TESTDATA("data_out/FastqDownsample_out1_Windows.txt");
		else if (Helper::isMacOS()) expected_file = TESTDATA("data_out/FastqDownsample_out1_OSX.txt");
		COMPARE_FILES(lastLogFile(), expected_file);
	}
};
