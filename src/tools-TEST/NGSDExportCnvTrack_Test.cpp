#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportCnvTrack_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportCnvTrack_init.sql"));

		//test
		EXECUTE("NGSDExportCnvTrack", "-test -system ssHAEv7 -out out/NGSDExportCnvTrack_out1.igv -stats out/NGSDExportCnvTrack_out1.log");
		COMPARE_FILES("out/NGSDExportCnvTrack_out1.igv", TESTDATA("data_out/NGSDExportCnvTrack_out1.igv"));
		COMPARE_FILES("out/NGSDExportCnvTrack_out1.log", TESTDATA("data_out/NGSDExportCnvTrack_out1.log"));
	}

	TEST_METHOD(with_qc_filtering)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportCnvTrack_init.sql"));

		//test
		EXECUTE("NGSDExportCnvTrack", "-test -system ssHAEv7 -out out/NGSDExportCnvTrack_out2.igv -stats out/NGSDExportCnvTrack_out2.log -min_dp 50 -min_af 0.51");
		COMPARE_FILES("out/NGSDExportCnvTrack_out2.igv", TESTDATA("data_out/NGSDExportCnvTrack_out2.igv"));
		COMPARE_FILES("out/NGSDExportCnvTrack_out2.log", TESTDATA("data_out/NGSDExportCnvTrack_out2.log"));
	}

};

