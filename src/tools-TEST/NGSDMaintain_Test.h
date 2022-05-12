#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDMaintain_Test)
{
Q_OBJECT
private slots:

	//NOTE: merged sample processing is not tested
	void check_and_fix()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDMaintain_in1.sql"));

		//test, no fix
		EXECUTE("NGSDMaintain", "-test -out out/NGSDMaintain_out1.log");
		COMPARE_FILES("out/NGSDMaintain_out1.log", TESTDATA("data_out/NGSDMaintain_out1.log"));

		//test, fix
		EXECUTE("NGSDMaintain", "-test -fix -out out/NGSDMaintain_out2.log");
		COMPARE_FILES("out/NGSDMaintain_out2.log", TESTDATA("data_out/NGSDMaintain_out2.log"));

		//test, nothing to fix
		EXECUTE("NGSDMaintain", "-test -fix -out out/NGSDMaintain_out3.log");
		COMPARE_FILES("out/NGSDMaintain_out3.log", TESTDATA("data_out/NGSDMaintain_out3.log"));

	}
};


