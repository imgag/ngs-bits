#include "TestFramework.h"

TEST_CLASS(EstimateTumorContent_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("EstimateTumorContent", "-tu " + TESTDATA("data_in/EstimateTumorContent_variants.tsv") + " -tu_bam " + TESTDATA("data_in/EstimateTumorContent_in1.bam") + " -no_bam " + TESTDATA("data_in/EstimateTumorContent_in2.bam") + " -out out/EstimateTumorContent_out1.txt");
		COMPARE_FILES("out/EstimateTumorContent_out1.txt", TESTDATA("data_out/EstimateTumorContent_out1.txt"));
	}

};
