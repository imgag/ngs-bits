#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(NGSDAnnotateGeneExpression_Test)
{
private:

	TEST_METHOD(germline)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateGeneExpression_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateGeneExpression", "-test -rna_ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateGeneExpression_in1.GSvar") + " -out out/NGSDAnnotateGeneExpression_out1.GSvar");
		COMPARE_FILES("out/NGSDAnnotateGeneExpression_out1.GSvar", TESTDATA("data_out/NGSDAnnotateGeneExpression_out1.GSvar"))
	}

	TEST_METHOD(germline_project)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateGeneExpression_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateGeneExpression", "-test -cohort_strategy RNA_COHORT_GERMLINE_PROJECT -rna_ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateGeneExpression_in1.GSvar")
				+ " -out out/NGSDAnnotateGeneExpression_out2.GSvar");
		COMPARE_FILES("out/NGSDAnnotateGeneExpression_out2.GSvar", TESTDATA("data_out/NGSDAnnotateGeneExpression_out2.GSvar"))
	}

	TEST_METHOD(somatic)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateGeneExpression_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateGeneExpression", "-test -cohort_strategy RNA_COHORT_SOMATIC -rna_ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateGeneExpression_in1.GSvar")
				+ " -out out/NGSDAnnotateGeneExpression_out3.GSvar");
		COMPARE_FILES("out/NGSDAnnotateGeneExpression_out3.GSvar", TESTDATA("data_out/NGSDAnnotateGeneExpression_out3.GSvar"))
	}


};


