#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(BedpeAnnotateCounts_Test)
{
private:

	TEST_METHOD(std_params)
	{
		//test
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedpeAnnotateCounts_init1.sql"));

		EXECUTE("BedpeAnnotateCounts", "-in " + TESTDATA("data_in/BedpeAnnotateCounts_in1.bedpe") + " -ann_folder " + TESTDATA("data_in/BedpeAnnotateCounts/")
				+ " -ps_name NA12878_3 -test -out out/BedpeAnnotateCounts_out1.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCounts_out1.bedpe", TESTDATA("data_out/BedpeAnnotateCounts_out1.bedpe"));
	}

	TEST_METHOD(no_sample_count)
	{
		//test
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedpeAnnotateCounts_init1.sql"));

		EXECUTE("BedpeAnnotateCounts", "-in " + TESTDATA("data_in/BedpeAnnotateCounts_in1.bedpe") + " -ann_folder " + TESTDATA("data_in/BedpeAnnotateCounts/")
				+ " -ps_name NA12878_3 -test -processing_system invalid -out out/BedpeAnnotateCounts_out2.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCounts_out2.bedpe", TESTDATA("data_out/BedpeAnnotateCounts_out2.bedpe"));
	}

	TEST_METHOD(no_ps_name_no_ngsd)
	{
		EXECUTE("BedpeAnnotateCounts", "-in " + TESTDATA("data_in/BedpeAnnotateCounts_in1.bedpe") + " -ann_folder " + TESTDATA("data_in/BedpeAnnotateCounts/")
				+ " -processing_system hpHBOCv5 -out out/BedpeAnnotateCounts_out3.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCounts_out3.bedpe", TESTDATA("data_out/BedpeAnnotateCounts_out3.bedpe"));
	}

};
