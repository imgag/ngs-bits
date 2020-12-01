#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(PhenotypesToGenes_Test)
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
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypesToGenes_init.sql"));

		//test
		EXECUTE("PhenotypesToGenes", "-test -in " + TESTDATA("data_in/PhenotypesToGenes_in1.txt") + " -ignore_invalid -out out/PhenotypesToGenes_out1.txt");
		COMPARE_FILES("out/PhenotypesToGenes_out1.txt", TESTDATA("data_out/PhenotypesToGenes_out1.txt"));
	}
};
