#include "TestFramework.h"

TEST_CLASS(BedpeSort_Test)
{
Q_OBJECT
private slots:

	void deafult_parameters()
	{
		EXECUTE("BedpeSort", "-in " + TESTDATA("data_in/BedpeSort_in1.bedpe") + " -out out/BedpeSort_out1.bedpe");
		COMPARE_FILES("out/BedpeSort_out1.bedpe", TESTDATA("data_out/BedpeSort_out1.bedpe"));
	}

};
