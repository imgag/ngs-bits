#include "TestFramework.h"

TEST_CLASS(BamFilter_Test)
{
Q_OBJECT
private slots:

	void test_02()
	{
		EXECUTE("BamFilter", "-in " + TESTDATA("data_in/BamFilter_in1.bam") + " -out out/BamFilter_out1.bam");
		COMPARE_GZ_FILES("out/BamFilter_out1.bam", TESTDATA("data_out/BamFilter_out1.bam"));
	}

	void test_01()
	{
		EXECUTE("BamFilter", "-in " + TESTDATA("data_in/BamFilter_in2.bam") + " -out out/BamFilter_out2.bam -minDup 4");
		COMPARE_GZ_FILES("out/BamFilter_out2.bam", TESTDATA("data_out/BamFilter_out2.bam"));
	}

};
