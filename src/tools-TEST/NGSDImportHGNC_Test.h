#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportHGNC_Test)
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

		//test
		EXECUTE("NGSDImportHGNC", "-test -in " + TESTDATA("data_in/NGSDImportHGNC_in1.txt"));
	}

};

