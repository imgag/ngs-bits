#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportGff_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init db
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGff_init.sql"));

		//test
		EXECUTE("NGSDExportGff", "-test -out out/NGSDExportGff_out1.gff3");
		COMPARE_FILES("out/NGSDExportGff_out1.gff3", TESTDATA("data_out/NGSDExportGff_out1.gff3"));
	}

	TEST_METHOD(with_genes)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init db
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGff_init.sql"));

		//test
		EXECUTE("NGSDExportGff", "-test -genes -out out/NGSDExportGff_out2.gff3");
		COMPARE_FILES("out/NGSDExportGff_out2.gff3", TESTDATA("data_out/NGSDExportGff_out2.gff3"));
	}
};

