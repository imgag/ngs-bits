#include "TestFramework.h"


TEST_CLASS(BamDownsample_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		EXECUTE("BamDownsample", "-in " + TESTDATA("data_in/BamDownsample_in1.bam") + " -out out/BamDownsample_out1.bam -percentage 80 -test");
		IS_TRUE(QFile::exists("out/BamDownsample_out1.bam"));
		COMPARE_FILES("out/BamDownsample_out1.txt", TESTDATA("data_out/BamDownsample_out1.txt"));
	}
};
