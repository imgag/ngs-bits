#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDAnnotateCNV_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateCNV_init.sql"));

		//test
		EXECUTE("NGSDAnnotateCNV", "-test -in "+ TESTDATA("data_in/NGSDAnnotateCNV_in.tsv") + " -out out/NGSDAnnotateCNV_out.tsv");

		COMPARE_FILES("out/NGSDAnnotateCNV_out.tsv", TESTDATA("data_out/NGSDAnnotateCNV_out.tsv"));
	}

};


