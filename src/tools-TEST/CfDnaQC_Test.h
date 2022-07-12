#include "TestFramework.h"

TEST_CLASS(CfDnaQC_Test)
{
Q_OBJECT
private slots:

	void base_test1()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -build hg19 -out out/CfDnaQC_out1.qcML");
		REMOVE_LINES("out/CfDnaQC_out1.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out1.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out1.qcML", TESTDATA("data_out/CfDnaQC_out1.qcML"));
	}

	void base_test2()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -build hg19 -out out/CfDnaQC_out2.qcML");
		REMOVE_LINES("out/CfDnaQC_out2.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out2.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out2.qcML", TESTDATA("data_out/CfDnaQC_out2.qcML"));
	}

	void text_test()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -build hg19 -txt -out out/CfDnaQC_out2.txt");
		COMPARE_FILES("out/CfDnaQC_out2.txt", TESTDATA("data_out/CfDnaQC_out2.txt"));
	}

	void tumor_test()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -tumor_bam " + TESTDATA("data_in/CfDnaQC_in_tumor.bam") + " -build hg19 -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out3.qcML");
		REMOVE_LINES("out/CfDnaQC_out3.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out3.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out3.qcML", TESTDATA("data_out/CfDnaQC_out3.qcML"));
	}

	void cfdna_relation_test()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -related_bams " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -build hg19 -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out4.qcML");
		REMOVE_LINES("out/CfDnaQC_out4.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out4.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out4.qcML", TESTDATA("data_out/CfDnaQC_out4.qcML"));
	}

	void error_rates_test()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -error_rates "
				+ TESTDATA("data_in/CfDnaQC_in_error_rates.tsv") + " -out out/CfDnaQC_out5.qcML");
		REMOVE_LINES("out/CfDnaQC_out5.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out5.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out5.qcML", TESTDATA("data_out/CfDnaQC_out5.qcML"));
	}

};
