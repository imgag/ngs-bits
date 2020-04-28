#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(BedpeToBed_Test)
{
Q_OBJECT
private slots:

	void std_params()
	{
		//test
		EXECUTE("BedpeToBed", "-in " + TESTDATA("data_in/BedpeToBed_in1.bedpe") + " -out out/BedpeToBed_out1.bed");

		COMPARE_FILES("out/BedpeToBed_out1.bed", TESTDATA("data_out/BedpeToBed_out1.bed"));
	}

};
