#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedpeAnnotateCounts_Test)
{
Q_OBJECT
private slots:

	void std_params()
	{
		//test
		EXECUTE("BedpeAnnotateCounts", "-in " + TESTDATA("data_in/BedpeAnnotateCounts_in1.bedpe") + " -ann_folder " + TESTDATA("data_in/BedpeAnnotateCounts/")
				+ " -processing_system ssHAEv6 -out out/BedpeAnnotateCounts_out1.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCounts_out1.bedpe", TESTDATA("data_out/BedpeAnnotateCounts_out1.bedpe"));
	}

	void no_sample_count()
	{
		//test
		EXECUTE("BedpeAnnotateCounts", "-in " + TESTDATA("data_in/BedpeAnnotateCounts_in1.bedpe") + " -ann_folder " + TESTDATA("data_in/BedpeAnnotateCounts/")
				+ " -processing_system invalid -out out/BedpeAnnotateCounts_out2.bedpe");

		COMPARE_FILES("out/BedpeAnnotateCounts_out2.bedpe", TESTDATA("data_out/BedpeAnnotateCounts_out2.bedpe"));
	}

};
