#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VariantAnnotateFrequency_Test)
{
Q_OBJECT
private slots:

	//Test with name and depth arguments
	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		TFW_EXEC("VariantAnnotateFrequency", "-in " + QFINDTESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out1.tsv -depth -name bla -ref " + ref_file);
		TFW::comareFiles("out/VariantAnnotateFrequency_out1.tsv", QFINDTESTDATA("data_out/VariantAnnotateFrequency_out1.tsv"));
	}
	

	//Test without arguments
	void test_02()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		TFW_EXEC("VariantAnnotateFrequency", "-in " + QFINDTESTDATA("data_in/VariantAnnotateFrequency_in1.tsv") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out2.tsv -ref " + ref_file);
		TFW::comareFiles("out/VariantAnnotateFrequency_out2.tsv", QFINDTESTDATA("data_out/VariantAnnotateFrequency_out2.tsv"));
	}

	//Test with mapq0 argument
	void test_03()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		TFW_EXEC("VariantAnnotateFrequency", "-in " + QFINDTESTDATA("data_in/VariantAnnotateFrequency_in2.tsv") + " -bam " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -out out/VariantAnnotateFrequency_out3.tsv -mapq0 -ref " + ref_file);
		TFW::comareFiles("out/VariantAnnotateFrequency_out3.tsv", QFINDTESTDATA("data_out/VariantAnnotateFrequency_out3.tsv"));
	}

};


