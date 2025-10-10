#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDAnnotateCNV_Test)
{
private:

	TEST_METHOD(test_01)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateCNV_init.sql"));

		//test
		EXECUTE("NGSDAnnotateCNV", "-test -in "+ TESTDATA("data_in/NGSDAnnotateCNV_in.tsv") + " -out out/NGSDAnnotateCNV_out.tsv");

		COMPARE_FILES("out/NGSDAnnotateCNV_out.tsv", TESTDATA("data_out/NGSDAnnotateCNV_out.tsv"));
	}

	TEST_METHOD(reannotate)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		//annotate with empty DB
		EXECUTE("NGSDAnnotateCNV", "-test -in "+ TESTDATA("data_in/NGSDAnnotateCNV_in.tsv") + " -out out/NGSDAnnotateCNV_out2_temp.tsv");

		// add DB entries
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateCNV_init.sql"));

		// reannotate
		EXECUTE("NGSDAnnotateCNV", "-test -in out/NGSDAnnotateCNV_out2_temp.tsv -out out/NGSDAnnotateCNV_out2.tsv");

		COMPARE_FILES("out/NGSDAnnotateCNV_out2.tsv", TESTDATA("data_out/NGSDAnnotateCNV_out.tsv"));
	}

};


