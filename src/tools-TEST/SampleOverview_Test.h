#include "TestFramework.h"

TEST_CLASS(SampleOverview_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("SampleOverview", "-in " + TESTDATA("data_in/SampleOverview_in1.tsv") + " " + TESTDATA("data_in/SampleOverview_in2.tsv") + " " + TESTDATA("data_in/SampleOverview_in3.tsv") + " -out out/SampleOverview_out1.tsv");
		COMPARE_FILES("out/SampleOverview_out1.tsv", TESTDATA("data_out/SampleOverview_out1.tsv"));
	}
	
	void test_02()
	{
		EXECUTE("SampleOverview", "-in " + TESTDATA("data_in/SampleOverview_in1.tsv") + " " + TESTDATA("data_in/SampleOverview_in2.tsv") + " " + TESTDATA("data_in/SampleOverview_in3.tsv") + " -out out/SampleOverview_out2.tsv -add_cols snp_snp132,ljb_phylop");
		COMPARE_FILES("out/SampleOverview_out2.tsv", TESTDATA("data_out/SampleOverview_out2.tsv"));
	}

};

