#include "TestFramework.h"

class TrioAnnotation_Test
		: public QObject
{
	Q_OBJECT

private slots:

	//Test with name and depth arguments
	void test_01()
	{
		TFW_EXEC("TrioAnnotation", "-in " + QFINDTESTDATA("data_in/TrioAnnotation_in1.GSvar") + " -out out/TrioAnnotation_out1.GSvar");
		TFW::comareFiles("out/TrioAnnotation_out1.GSvar", QFINDTESTDATA("data_out/TrioAnnotation_out1.GSvar"));
	}

};

TFW_DECLARE(TrioAnnotation_Test)


