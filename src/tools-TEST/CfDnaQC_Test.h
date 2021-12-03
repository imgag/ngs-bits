#include "TestFramework.h"

TEST_CLASS(CfDnaQC_Test)
{
Q_OBJECT
private slots:

	void base_test1()
	{
		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out1.qcML");
		REMOVE_LINES("out/CfDnaQC_out1.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out1.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out1.qcML", TESTDATA("data_out/CfDnaQC_out1.qcML"));
	}

	void base_test2()
	{
		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out2.qcML");
		REMOVE_LINES("out/CfDnaQC_out2.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out2.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out2.qcML", TESTDATA("data_out/CfDnaQC_out2.qcML"));
	}

	void tumor_test()
	{
		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -tumor_bam " + TESTDATA("data_in/CfDnaQC_in_tumor.bam") + " -cfdna_panel "
				+ TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out3.qcML");
		REMOVE_LINES("out/CfDnaQC_out3.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out3.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out3.qcML", TESTDATA("data_out/CfDnaQC_out3.qcML"));
	}

	void cfdna_relation_test()
	{
		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -related_bams " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -cfdna_panel "
				+ TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out4.qcML");
		REMOVE_LINES("out/CfDnaQC_out4.qcML", QRegExp("creation "));
		REMOVE_LINES("out/CfDnaQC_out4.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out4.qcML", TESTDATA("data_out/CfDnaQC_out4.qcML"));
	}

};
