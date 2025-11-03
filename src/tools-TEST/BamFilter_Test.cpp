#include "TestFrameworkNGS.h"

TEST_CLASS(BamFilter_Test)
{
private:

	TEST_METHOD(no_filtering)
	{
		EXECUTE("BamFilter", "-in " + TESTDATA("data_in/BamFilter_in1.bam") + " -out out/BamFilter_out1.bam");

		BAM_TO_TEXT("out/BamFilter_out1.bam", "out/BamFilter_out1.bam.txt");
		BAM_TO_TEXT(TESTDATA("data_out/BamFilter_out1.bam"), "out/BamFilter_out1.expected.txt");
		COMPARE_FILES("out/BamFilter_out1.bam.txt", "out/BamFilter_out1.expected.txt");
	}

	TEST_METHOD(mq_filter)
	{
		EXECUTE("BamFilter", "-in " + TESTDATA("data_in/BamFilter_in2.bam") + " -out out/BamFilter_out2.bam -minMQ 50");

		BAM_TO_TEXT("out/BamFilter_out2.bam", "out/BamFilter_out2.bam.txt");
		BAM_TO_TEXT(TESTDATA("data_out/BamFilter_out2.bam"), "out/BamFilter_out2.expected.txt");
		COMPARE_FILES("out/BamFilter_out2.bam.txt", "out/BamFilter_out2.expected.txt");
	}

};
