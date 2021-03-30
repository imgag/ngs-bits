#include "TestFramework.h"

TEST_CLASS(SampleOverview_Test)
{
Q_OBJECT
private slots:
	
	void test_default()
	{
		EXECUTE("SampleOverview", "-in " + TESTDATA("data_in/SampleOverview_in1.GSvar") + " " + TESTDATA("data_in/SampleOverview_in2.GSvar") + " " + TESTDATA("data_in/SampleOverview_in3.GSvar") + " -out out/SampleOverview_out1.GSvar");
		COMPARE_FILES("out/SampleOverview_out1.GSvar", TESTDATA("data_out/SampleOverview_out1.GSvar"));
	}
	
	void test_addcols()
	{
		EXECUTE("SampleOverview", "-in " + TESTDATA("data_in/SampleOverview_in1.GSvar") + " " + TESTDATA("data_in/SampleOverview_in2.GSvar") + " " + TESTDATA("data_in/SampleOverview_in3.GSvar") + " -out out/SampleOverview_out2.GSvar -add_cols genotype,variant_type,gene,coding_and_splicing,snp_snp132,ljb_phylop");
		COMPARE_FILES("out/SampleOverview_out2.GSvar", TESTDATA("data_out/SampleOverview_out2.GSvar"));
	}

};

