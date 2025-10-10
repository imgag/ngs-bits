#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(CfDnaQC_Test)
{
private:

	TEST_METHOD(base_test1)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -build hg19 -out out/CfDnaQC_out1.qcML");
        REMOVE_LINES("out/CfDnaQC_out1.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/CfDnaQC_out1.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out1.qcML", TESTDATA("data_out/CfDnaQC_out1.qcML"));
	}

	TEST_METHOD(base_test2)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -build hg19 -out out/CfDnaQC_out2.qcML");
        REMOVE_LINES("out/CfDnaQC_out2.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/CfDnaQC_out2.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out2.qcML", TESTDATA("data_out/CfDnaQC_out2.qcML"));
	}

	TEST_METHOD(text_test)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -build hg19 -txt -out out/CfDnaQC_out2.txt");
		COMPARE_FILES("out/CfDnaQC_out2.txt", TESTDATA("data_out/CfDnaQC_out2.txt"));
	}

	TEST_METHOD(tumor_test)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -tumor_bam " + TESTDATA("data_in/CfDnaQC_in_tumor.bam") + " -build hg19 -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out3.qcML");
        REMOVE_LINES("out/CfDnaQC_out3.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/CfDnaQC_out3.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out3.qcML", TESTDATA("data_out/CfDnaQC_out3.qcML"));
	}

	TEST_METHOD(cfdna_relation_test)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -related_bams " + TESTDATA("data_in/CfDnaQC_in_cfdna2.bam") + " -build hg19 -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -out out/CfDnaQC_out4.qcML");
        REMOVE_LINES("out/CfDnaQC_out4.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/CfDnaQC_out4.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out4.qcML", TESTDATA("data_out/CfDnaQC_out4.qcML"));
	}

	TEST_METHOD(error_rates_test)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("CfDnaQC", "-bam " + TESTDATA("data_in/CfDnaQC_in_cfdna1.bam") + " -cfdna_panel " + TESTDATA("data_in/CfDnaQC_in_panel.bed") + " -error_rates "
				+ TESTDATA("data_in/CfDnaQC_in_error_rates.tsv") + " -out out/CfDnaQC_out5.qcML");
        REMOVE_LINES("out/CfDnaQC_out5.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/CfDnaQC_out5.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/CfDnaQC_out5.qcML", TESTDATA("data_out/CfDnaQC_out5.qcML"));
	}

};
