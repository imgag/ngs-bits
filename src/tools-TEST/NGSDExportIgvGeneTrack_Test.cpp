#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportIgvGeneTrack_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

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

