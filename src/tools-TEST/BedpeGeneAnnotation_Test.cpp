#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(BedpeGeneAnnotation_Test)
{
private:

	TEST_METHOD(std_params)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedpeGeneAnnotation_init.sql"));

		//test
		EXECUTE("BedpeGeneAnnotation", "-test -in " + TESTDATA("data_in/BedpeGeneAnnotation_in1.bedpe") + " -out out/BedpeGeneAnnotation_out1.bedpe");

		COMPARE_FILES("out/BedpeGeneAnnotation_out1.bedpe", TESTDATA("data_out/BedpeGeneAnnotation_out1.bedpe"));
	}

	TEST_METHOD(add_gene_name_col)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedpeGeneAnnotation_init.sql"));

		//test
		EXECUTE("BedpeGeneAnnotation", "-add_simple_gene_names -test -in " + TESTDATA("data_in/BedpeGeneAnnotation_in1.bedpe") + " -out out/BedpeGeneAnnotation_out2.bedpe");

		COMPARE_FILES("out/BedpeGeneAnnotation_out2.bedpe", TESTDATA("data_out/BedpeGeneAnnotation_out2.bedpe"));
	}

	TEST_METHOD(reannotate)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedpeGeneAnnotation_init.sql"));

		//test
		EXECUTE("BedpeGeneAnnotation", "-add_simple_gene_names -test -in " + TESTDATA("data_in/BedpeGeneAnnotation_in2.bedpe") + " -out out/BedpeGeneAnnotation_out3.bedpe");

		COMPARE_FILES("out/BedpeGeneAnnotation_out3.bedpe", TESTDATA("data_out/BedpeGeneAnnotation_out2.bedpe"));
	}



};


