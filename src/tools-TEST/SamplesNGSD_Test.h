#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(SamplesNGSD_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplesNGSD_init.sql"));

		//test
		EXECUTE("SamplesNGSD", "-test -out out/SamplesNGSD_out1.tsv");
		COMPARE_FILES("out/SamplesNGSD_out1.tsv", TESTDATA("data_out/SamplesNGSD_out1.tsv"));
	}

	void with_qc_flag()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplesNGSD_init.sql"));

		//test
		EXECUTE("SamplesNGSD", "-test -qc -out out/SamplesNGSD_out2.tsv");
		COMPARE_FILES("out/SamplesNGSD_out2.tsv", TESTDATA("data_out/SamplesNGSD_out2.tsv"));
	}

	void with_check_path_flag()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplesNGSD_init.sql"));

		//test
		EXECUTE("SamplesNGSD", "-test -check_path -out out/SamplesNGSD_out3.tsv");
		COMPARE_FILES("out/SamplesNGSD_out3.tsv", TESTDATA("data_out/SamplesNGSD_out3.tsv"));
	}


};

