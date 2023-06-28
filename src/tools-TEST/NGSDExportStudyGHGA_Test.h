#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportStudyGHGA_Test)
{
Q_OBJECT
private slots:

	void test_default()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -test -out out/NGSDExportStudyGHGA_out1.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out1.json", TESTDATA("data_out/NGSDExportStudyGHGA_out1.json"));
	}

};
