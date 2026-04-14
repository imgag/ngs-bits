#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportSpliceAI_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init db
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSpliceAI_init.sql"));

		//test
		EXECUTE("NGSDExportSpliceAI", "-test -out out/NGSDExportSpliceAI_out1.txt");
		COMPARE_FILES("out/NGSDExportSpliceAI_out1.txt", TESTDATA("data_out/NGSDExportSpliceAI_out1.txt"));
	}
};

