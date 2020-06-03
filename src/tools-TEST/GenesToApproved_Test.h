#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(GenesToApproved_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		EXECUTE("NGSDImportHGNC", "-test -in " + TESTDATA("data_in/NGSDImportHGNC_in1.txt"));

		//test
		EXECUTE("GenesToApproved", "-test -in " + TESTDATA("data_in/GenesToApproved_in1.txt") + " -out out/GenesToApproved_out1.txt");
		COMPARE_FILES("out/GenesToApproved_out1.txt", TESTDATA("data_out/GenesToApproved_out1.txt"));
	}

	void with_report_ambiguous()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		EXECUTE("NGSDImportHGNC", "-test -in " + TESTDATA("data_in/NGSDImportHGNC_in1.txt"));

		//test
		EXECUTE("GenesToApproved", "-test -in " + TESTDATA("data_in/GenesToApproved_in1.txt") + " -report_ambiguous -out out/GenesToApproved_out2.txt");
		COMPARE_FILES("out/GenesToApproved_out2.txt", TESTDATA("data_out/GenesToApproved_out2.txt"));
	}
};

