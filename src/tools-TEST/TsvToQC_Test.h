#include "TestFramework.h"

TEST_CLASS(TsvToQC_Test)
{
Q_OBJECT
private slots:
	
	//test with column names
	void test_01()
	{
		EXECUTE("TsvToQC", "-in " + TESTDATA("data_in/TsvToQC_in1.tsv") + " -sources " + TESTDATA("data_in/TsvToQC_in1.tsv") + " -out out/TsvToQC_out1.qcML");
		REMOVE_LINES("out/TsvToQC_out1.qcML", QRegExp("creation "));
		COMPARE_FILES("out/TsvToQC_out1.qcML", TESTDATA("data_out/TsvToQC_out1.qcML"));
	}
};

