#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(NGSDAnnotateRNA_Test)
{
private:


	TEST_METHOD(germline)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateRNA", "-test -ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv") + " -out out/NGSDAnnotateRNA_expr_out1.tsv -corr out/NGSDAnnotateRNA_corr_out1.txt");
		COMPARE_FILES("out/NGSDAnnotateRNA_expr_out1.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_out1.tsv"));
		COMPARE_FILES("out/NGSDAnnotateRNA_corr_out1.txt", TESTDATA("data_out/NGSDAnnotateRNA_corr_out1.txt"));
	}


	TEST_METHOD(germline_project)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateRNA", "-test -cohort_strategy RNA_COHORT_GERMLINE_PROJECT -ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv") + " -out out/NGSDAnnotateRNA_expr_out2.tsv");
		COMPARE_FILES("out/NGSDAnnotateRNA_expr_out2.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_out2.tsv"))
	}


	TEST_METHOD(somatic)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateRNA", "-test -cohort_strategy RNA_COHORT_SOMATIC -ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv") + " -out out/NGSDAnnotateRNA_expr_out3.tsv");
		COMPARE_FILES("out/NGSDAnnotateRNA_expr_out3.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_out3.tsv"))
	}


	TEST_METHOD(germline_exon)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in1.tsv"), "RX001_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in2.tsv"), "RX002_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in3.tsv"), "RX003_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in4.tsv"), "RX004_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in5.tsv"), "RX005_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in6.tsv"), "RX006_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in7.tsv"), "RX007_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateRNA", "-test -ps RX001_01 -mode exons -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in1.tsv") + " -out out/NGSDAnnotateRNA_expr_out4.tsv");
		COMPARE_FILES("out/NGSDAnnotateRNA_expr_out4.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_exon_out4.tsv"))
	}


	TEST_METHOD(somatic_hpa)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateRNA", "-test -cohort_strategy RNA_COHORT_SOMATIC -ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv")
				+ " -hpa_file " + TESTDATA("data_in/NGSDAnnotateRNA_in_hpa.tsv") + " -out out/NGSDAnnotateRNA_expr_out5.tsv");
		COMPARE_FILES("out/NGSDAnnotateRNA_expr_out5.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_out5.tsv"))
	}


	TEST_METHOD(germline_update_genes)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateRNA", "-test -ps RX001_01 -update_genes -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv") + " -out out/NGSDAnnotateRNA_expr_out6.tsv");
		COMPARE_FILES("out/NGSDAnnotateRNA_expr_out6.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_out6.tsv"));
	}


	TEST_METHOD(germline_exon_update_genes)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in1.tsv"), "RX001_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in2.tsv"), "RX002_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in3.tsv"), "RX003_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in4.tsv"), "RX004_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in5.tsv"), "RX005_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in6.tsv"), "RX006_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in7.tsv"), "RX007_01", false, false);
		db.importExonExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in8.tsv"), "RX008_01", false, false);

		EXECUTE("NGSDAnnotateRNA", "-test -ps RX001_01 -update_genes -mode exons -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_exon_in1.tsv") + " -out out/NGSDAnnotateRNA_expr_out7.tsv");
		COMPARE_FILES("out/NGSDAnnotateRNA_expr_out7.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_exon_out7.tsv"))
	}

	TEST_METHOD(somatic_hpa_with_cohort_file)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateRNA_NGSD_init.sql"));

		//import test data
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv"), "RX001_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in2.tsv"), "RX002_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in3.tsv"), "RX003_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in4.tsv"), "RX004_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in5.tsv"), "RX005_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in6.tsv"), "RX006_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in7.tsv"), "RX007_01", false, false);
		db.importGeneExpressionData(TESTDATA("data_in/NGSDAnnotateRNA_expr_in8.tsv"), "RX008_01", false, false);

		//uses same data as is imported only as file
		EXECUTE("NGSDAnnotateRNA", "-test -cohort_strategy RNA_COHORT_SOMATIC -ps RX001_01 -in " + TESTDATA("data_in/NGSDAnnotateRNA_expr_in1.tsv")
				+ " -hpa_file " + TESTDATA("data_in/NGSDAnnotateRNA_in_hpa.tsv") + " -cohort_data " + TESTDATA("data_in/NGSDAnnotateRNA_expr_cohort_in1.tsv") + " -out out/NGSDAnnotateRNA_expr_out8.tsv");
		COMPARE_FILES_DELTA("out/NGSDAnnotateRNA_expr_out8.tsv", TESTDATA("data_out/NGSDAnnotateRNA_expr_out5.tsv"), 0.01, false, '\t')
	}

};


