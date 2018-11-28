#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportSamples_Test)
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
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -out out/NGSDExportSamples_out1.tsv");
		COMPARE_FILES("out/NGSDExportSamples_out1.tsv", TESTDATA("data_out/NGSDExportSamples_out1.tsv"));
	}

	void with_disease_details_and_qc_and_outcome_flags()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -disease_details -outcome -qc -out out/NGSDExportSamples_out2.tsv");
		COMPARE_FILES("out/NGSDExportSamples_out2.tsv", TESTDATA("data_out/NGSDExportSamples_out2.tsv"));
	}

	void with_check_path_flag()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -check_path -out out/NGSDExportSamples_out3.tsv");
		COMPARE_FILES("out/NGSDExportSamples_out3.tsv", TESTDATA("data_out/NGSDExportSamples_out3.tsv"));
	}


};

