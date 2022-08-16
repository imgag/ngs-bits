#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExtractRNACohort_Test)
{
Q_OBJECT
private slots:

	void germline()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExtractRNACohort_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDExtractRNACohort", "-test -ps RX001_01 -genes " + TESTDATA("data_in/NGSDExtractRNACohort_genes.txt") + " -out out/NGSDExtractRNACohort_cohort_out1.tsv");
		COMPARE_FILES("out/NGSDExtractRNACohort_cohort_out1.tsv", TESTDATA("data_out/NGSDExtractRNACohort_cohort_out1.tsv"))
	}

	void germline_project()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExtractRNACohort_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDExtractRNACohort", "-test -ps RX001_01 -cohort_strategy RNA_COHORT_GERMLINE_PROJECT -genes " + TESTDATA("data_in/NGSDExtractRNACohort_genes.txt")
				+ " -out out/NGSDExtractRNACohort_cohort_out2.tsv");
		COMPARE_FILES("out/NGSDExtractRNACohort_cohort_out2.tsv", TESTDATA("data_out/NGSDExtractRNACohort_cohort_out2.tsv"))
	}

	void somatic()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExtractRNACohort_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDExtractRNACohort", "-test -ps RX001_01 -cohort_strategy RNA_COHORT_SOMATIC -genes " + TESTDATA("data_in/NGSDExtractRNACohort_genes.txt")
				+ " -out out/NGSDExtractRNACohort_cohort_out3.tsv");
		COMPARE_FILES("out/NGSDExtractRNACohort_cohort_out3.tsv", TESTDATA("data_out/NGSDExtractRNACohort_cohort_out3.tsv"))
	}

	void germline_sample_file()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExtractRNACohort_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDExtractRNACohort_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDExtractRNACohort", "-test -ps RX001_01 -genes " + TESTDATA("data_in/NGSDExtractRNACohort_genes.txt") + " -sample_expression "
				+ TESTDATA("data_in/NGSDExtractRNACohort_expr_in1.tsv") + " -out out/NGSDExtractRNACohort_cohort_out4.tsv");
		COMPARE_FILES("out/NGSDExtractRNACohort_cohort_out4.tsv", TESTDATA("data_out/NGSDExtractRNACohort_cohort_out4.tsv"))
	}






};


