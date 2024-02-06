#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportIgvGeneTrack_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init db
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportIgvGeneTrack_init.sql"));

		//test
		EXECUTE("NGSDExportIgvGeneTrack", "-test -out out/NGSDExportIgvGeneTrack_out1.txt -out_mane out/NGSDExportIgvGeneTrack_out2.txt");
		COMPARE_FILES("out/NGSDExportIgvGeneTrack_out1.txt", TESTDATA("data_out/NGSDExportIgvGeneTrack_out1.txt"));
		COMPARE_FILES("out/NGSDExportIgvGeneTrack_out2.txt", TESTDATA("data_out/NGSDExportIgvGeneTrack_out2.txt"));
	}

};

