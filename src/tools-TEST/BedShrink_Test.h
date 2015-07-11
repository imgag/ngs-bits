#include "TestFramework.h"

TEST_CLASS(BedShrink_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("BedShrink", "-in " + QFINDTESTDATA("data_in/BedShrink_in1.bed") + " -out out/BedShrink_test01_out.bed -n 25");
		TFW::comareFiles("out/BedShrink_test01_out.bed", QFINDTESTDATA("data_out/BedShrink_test01_out.bed"));
	}
	
};
