#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDInit_Test)
{
Q_OBJECT
private slots:
	
	void import_NA12878_03()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//test
		EXECUTE("NGSDInit", "-test -add " + TESTDATA("data_in/NGSDInit_in1.sql"));
	}
};


