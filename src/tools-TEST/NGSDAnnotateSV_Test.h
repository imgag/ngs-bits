#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDAnnotateSV_Test)
{
Q_OBJECT
private slots:

	void artifical_svs_new_sample()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateSV_init.sql"));

		EXECUTE("NGSDAnnotateSV", "-test -in " + TESTDATA("data_in/NGSDAnnotateSV_in1.bedpe") + " -out out/NGSDAnnotateSV_out1.bedpe");

		COMPARE_FILES("out/NGSDAnnotateSV_out1.bedpe", TESTDATA("data_out/NGSDAnnotateSV_out1.bedpe"))
	}
};


