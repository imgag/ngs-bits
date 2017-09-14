#include "TestFramework.h"

TEST_CLASS(TrioAnnotation_Test)
{
Q_OBJECT
private slots:

	void test_female()
	{
		EXECUTE("TrioAnnotation", "-in " + TESTDATA("data_in/TrioAnnotation_in1.GSvar") + " -out out/TrioAnnotation_out1.GSvar -gender female");
		COMPARE_FILES("out/TrioAnnotation_out1.GSvar", TESTDATA("data_out/TrioAnnotation_out1.GSvar"));
	}

	void test_male()
	{
		EXECUTE("TrioAnnotation", "-in " + TESTDATA("data_in/TrioAnnotation_in1.GSvar") + " -out out/TrioAnnotation_out2.GSvar -gender male");
		COMPARE_FILES("out/TrioAnnotation_out2.GSvar", TESTDATA("data_out/TrioAnnotation_out2.GSvar"));
	}

};



