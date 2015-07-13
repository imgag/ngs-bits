#include "TestFramework.h"

TEST_CLASS(TrioAnnotation_Test)
{
Q_OBJECT
private slots:

	//Test with name and depth arguments
	void test_01()
	{
		EXECUTE("TrioAnnotation", "-in " + TESTDATA("data_in/TrioAnnotation_in1.GSvar") + " -out out/TrioAnnotation_out1.GSvar");
		COMPARE_FILES("out/TrioAnnotation_out1.GSvar", TESTDATA("data_out/TrioAnnotation_out1.GSvar"));
	}

};



