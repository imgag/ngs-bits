#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(CnvGeneAnnotation_Test)
{
private:

	TEST_METHOD(default_parameter)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/CnvGeneAnnotation_init.sql"));

		//test
		EXECUTE("CnvGeneAnnotation", "-test -in " + TESTDATA("data_in/CnvGeneAnnotation_in.tsv") + " -out out/CnvGeneAnnotation_out1.tsv");

		COMPARE_FILES("out/CnvGeneAnnotation_out1.tsv", TESTDATA("data_out/CnvGeneAnnotation_out1.tsv"));
	}

	TEST_METHOD(add_simple_gene_names)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/CnvGeneAnnotation_init.sql"));

		//test
		EXECUTE("CnvGeneAnnotation", "-add_simple_gene_names -test -in " + TESTDATA("data_in/CnvGeneAnnotation_in.tsv") + " -out out/CnvGeneAnnotation_out2.tsv");

		COMPARE_FILES("out/CnvGeneAnnotation_out2.tsv", TESTDATA("data_out/CnvGeneAnnotation_out2.tsv"));
	}

	TEST_METHOD(reannotate)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();

		//annotate with empty DB
		EXECUTE("CnvGeneAnnotation", "-test -in " + TESTDATA("data_in/CnvGeneAnnotation_in.tsv") + " -out out/CnvGeneAnnotation_out2_temp.tsv");

		// add DB entries
		db.executeQueriesFromFile(TESTDATA("data_in/CnvGeneAnnotation_init.sql"));

		//reannotate
		EXECUTE("CnvGeneAnnotation", "-test -in out/CnvGeneAnnotation_out2_temp.tsv -out out/CnvGeneAnnotation_out2.tsv");

		COMPARE_FILES("out/CnvGeneAnnotation_out2.tsv", TESTDATA("data_out/CnvGeneAnnotation_out1.tsv"));
	}


};


