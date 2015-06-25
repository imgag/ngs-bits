#include "../TestFramework.h"

class SampleOverview_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("SampleOverview", "-in " + QFINDTESTDATA("data_in/SampleOverview_in1.tsv") + " " + QFINDTESTDATA("data_in/SampleOverview_in2.tsv") + " " + QFINDTESTDATA("data_in/SampleOverview_in3.tsv") + " -out out/SampleOverview_out1.tsv");
		TFW::comareFiles("out/SampleOverview_out1.tsv", QFINDTESTDATA("data_out/SampleOverview_out1.tsv"));
	}
	
	void test_02()
	{
		TFW_EXEC("SampleOverview", "-in " + QFINDTESTDATA("data_in/SampleOverview_in1.tsv") + " " + QFINDTESTDATA("data_in/SampleOverview_in2.tsv") + " " + QFINDTESTDATA("data_in/SampleOverview_in3.tsv") + " -out out/SampleOverview_out2.tsv -add_cols snp_snp132,ljb_phylop");
		TFW::comareFiles("out/SampleOverview_out2.tsv", QFINDTESTDATA("data_out/SampleOverview_out2.tsv"));
	}

};

TFW_DECLARE(SampleOverview_Test)

