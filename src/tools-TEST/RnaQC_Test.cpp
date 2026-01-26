#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(RnaQC_Test)
{
private:

	TEST_METHOD(base_test1)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("RnaQC", "-bam " + TESTDATA("data_in/RnaQC_in1.bam")
				+ " -housekeeping_genes " + TESTDATA("data_in/RnaQC_in1_housekeeping_genes.bed")
				+ " -out out/RnaQC_out1.qcML");
        REMOVE_LINES("out/RnaQC_out1.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/RnaQC_out1.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/RnaQC_out1.qcML", TESTDATA("data_out/RnaQC_out1.qcML"));
	}

	TEST_METHOD(base_test2)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("RnaQC", "-bam " + TESTDATA("data_in/RnaQC_in1.bam")
				+ " -housekeeping_genes " + TESTDATA("data_in/RnaQC_in1_housekeeping_genes.bed")
				+ " -splicing " + TESTDATA("data_in/RnaQC_in1_splicing_gene.tsv")
				+ " -expression " + TESTDATA("data_in/RnaQC_in1_expr.tsv")
				+ " -out out/RnaQC_out2.qcML");
        REMOVE_LINES("out/RnaQC_out2.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/RnaQC_out2.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/RnaQC_out2.qcML", TESTDATA("data_out/RnaQC_out2.qcML"));
	}

	TEST_METHOD(base_test3)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("RnaQC", "-bam " + TESTDATA("data_in/RnaQC_in1.bam")
				+ " -splicing " + TESTDATA("data_in/RnaQC_in1_splicing_gene.tsv")
				+ " -expression " + TESTDATA("data_in/RnaQC_in1_expr.tsv")
				+ " -out out/RnaQC_out3.qcML");
        REMOVE_LINES("out/RnaQC_out3.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/RnaQC_out3.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/RnaQC_out3.qcML", TESTDATA("data_out/RnaQC_out3.qcML"));
	}

	TEST_METHOD(base_test4)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("RnaQC", "-bam " + TESTDATA("data_in/RnaQC_in1.bam")
				+ " -housekeeping_genes " + TESTDATA("data_in/RnaQC_in1_housekeeping_genes.bed")
				+ " -roi " + TESTDATA("data_in/RnaQC_in1_roi.bed")
				+ " -splicing " + TESTDATA("data_in/RnaQC_in1_splicing_gene.tsv")
				+ " -expression " + TESTDATA("data_in/RnaQC_in1_expr.tsv")
				+ " -out out/RnaQC_out4.qcML");
        REMOVE_LINES("out/RnaQC_out4.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/RnaQC_out4.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/RnaQC_out4.qcML", TESTDATA("data_out/RnaQC_out4.qcML"));
	}

	TEST_METHOD(base_test5)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("RnaQC", "-bam " + TESTDATA("data_in/RnaQC_in1.bam")
				+ " -housekeeping_genes " + TESTDATA("data_in/RnaQC_in1_housekeeping_genes.bed")
				+ " -roi " + TESTDATA("data_in/RnaQC_in1_roi2.bed")
				+ " -splicing " + TESTDATA("data_in/RnaQC_in1_splicing_gene.tsv")
				+ " -expression " + TESTDATA("data_in/RnaQC_in1_expr.tsv")
				+ " -out out/RnaQC_out5.qcML");
        REMOVE_LINES("out/RnaQC_out5.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/RnaQC_out5.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/RnaQC_out5.qcML", TESTDATA("data_out/RnaQC_out5.qcML"));
	}

	TEST_METHOD(text_test)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("RnaQC", "-bam " + TESTDATA("data_in/RnaQC_in1.bam")
				+ " -housekeeping_genes " + TESTDATA("data_in/RnaQC_in1_housekeeping_genes.bed")
				+ " -splicing " + TESTDATA("data_in/RnaQC_in1_splicing_gene.tsv")
				+ " -expression " + TESTDATA("data_in/RnaQC_in1_expr.tsv")
				+ " -txt -out out/RnaQC_out2.txt");
        REMOVE_LINES("out/RnaQC_out2.txt", QRegularExpression("creation "));
        REMOVE_LINES("out/RnaQC_out2.txt", QRegularExpression("<binary>"));
		COMPARE_FILES("out/RnaQC_out2.txt", TESTDATA("data_out/RnaQC_out2.txt"));
	}



};
