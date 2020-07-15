#include "TestFramework.h"


TEST_CLASS(FastqDownsample_Test)
{
Q_OBJECT
private slots:

	void paired_end()
	{
		EXECUTE("FastqDownsample", "-in1 " + TESTDATA("data_in/FastqDownsample_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/FastqDownsample_in2.fastq.gz") + " -out1 out/FastqDownsample_out1.fastq.gz -out2 out/FastqDownsample_out2.fastq.gz -percentage 20 -test");
		IS_TRUE(QFile::exists("out/FastqDownsample_out1.fastq.gz"));

        //The random number generator behaves differently under Linux/OSX/Windows => we need separate expected test results
		#ifdef _WIN32
			COMPARE_FILES("out/FastqDownsample_Test_line11.log", TESTDATA("data_out/FastqDownsample_out1_Windows.txt"));
        #elif __APPLE__
			COMPARE_FILES("out/FastqDownsample_Test_line11.log", TESTDATA("data_out/FastqDownsample_out1_OSX.txt"));
		#else
			COMPARE_FILES("out/FastqDownsample_Test_line11.log", TESTDATA("data_out/FastqDownsample_out1_Linux.txt"));
		#endif

	}
};
