#include "TestFramework.h"

TEST_CLASS(BedAnnotateFreq_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("BedAnnotateFreq", "-in " + QFINDTESTDATA("data_in/BedAnnotateFreq_in1.bed") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/BedAnnotateFreq_test01_out.tsv");
		TFW::comareFiles("out/BedAnnotateFreq_test01_out.tsv", QFINDTESTDATA("data_out/BedAnnotateFreq_test01_out.tsv"));
	}
	
};
