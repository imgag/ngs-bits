#include "TestFramework.h"

TEST_CLASS(SampleGender_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("SampleGender", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method xy -out out/SampleGender_test01_out.txt");
		TFW::comareFiles("out/SampleGender_test01_out.txt", QFINDTESTDATA("data_out/SampleGender_test01_out.txt"));
	
		TFW_EXEC("SampleGender", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method hetx -out out/SampleGender_test02_out.txt");
		TFW::comareFiles("out/SampleGender_test02_out.txt", QFINDTESTDATA("data_out/SampleGender_test02_out.txt"));
	}

};

