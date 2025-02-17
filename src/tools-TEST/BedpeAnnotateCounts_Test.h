#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(BedpeAnnotateCounts_Test)
{
Q_OBJECT
private slots:

	void std_params()
	{
		//test
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedpeAnnotateCounts_init1.sql"));

		EXECUTE("BedpeAnnotateCounts", "-in " + TESTDATA("data_in/BedpeAnnotateCounts_in1.bedpe") + " -ann_folder " + TESTDATA("data_in/BedpeAnnotateCounts/")
				+ " -ps_name NA12878_3 -test -out out/BedpeAnnotateCounts_out1.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCounts_out1.bedpe", TESTDATA("data_out/BedpeAnnotateCounts_out1.bedpe"));
	}

	void no_sample_count()
	{
		//test
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedpeAnnotateCounts_init1.sql"));

		EXECUTE("BedpeAnnotateCounts", "-in " + TESTDATA("data_in/BedpeAnnotateCounts_in1.bedpe") + " -ann_folder " + TESTDATA("data_in/BedpeAnnotateCounts/")
				+ " -ps_name NA12878_3 -test -processing_system invalid -out out/BedpeAnnotateCounts_out2.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCounts_out2.bedpe", TESTDATA("data_out/BedpeAnnotateCounts_out2.bedpe"));
	}

};
