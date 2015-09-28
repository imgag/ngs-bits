#include "TestFramework.h"

TEST_CLASS(SampleGender_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method xy -out out/SampleGender_test01_out.txt");
		COMPARE_FILES("out/SampleGender_test01_out.txt", TESTDATA("data_out/SampleGender_test01_out.txt"));
	
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method hetx -out out/SampleGender_test02_out.txt");
		COMPARE_FILES("out/SampleGender_test02_out.txt", TESTDATA("data_out/SampleGender_test02_out.txt"));

		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method sry -out out/SampleGender_test03_out.txt");
		COMPARE_FILES("out/SampleGender_test03_out.txt", TESTDATA("data_out/SampleGender_test03_out.txt"));
	}

};

