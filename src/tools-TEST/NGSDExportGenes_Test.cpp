#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportGenes_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGenes_init.sql"));

		//test
		EXECUTE("NGSDExportGenes", "-test -out out/NGSDExportGenes_out1.tsv");
		COMPARE_FILES("out/NGSDExportGenes_out1.tsv", TESTDATA("data_out/NGSDExportGenes_out1.tsv"));
	}

	TEST_METHOD(with_disease_info)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGenes_init.sql"));

		//test
		EXECUTE("NGSDExportGenes", "-test -add_disease_info -out out/NGSDExportGenes_out2.tsv");
		COMPARE_FILES("out/NGSDExportGenes_out2.tsv", TESTDATA("data_out/NGSDExportGenes_out2.tsv"));
	}
};

