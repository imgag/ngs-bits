#include "TestFramework.h"

TEST_CLASS(SampleGender_Test)
{
Q_OBJECT
private slots:
	
	void method_xy()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method xy -out out/SampleGender_test01_out.tsv");
		COMPARE_FILES("out/SampleGender_test01_out.tsv", TESTDATA("data_out/SampleGender_test01_out.tsv"));
	}

	void method_hetx()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method hetx -out out/SampleGender_test02_out.tsv");
		COMPARE_FILES("out/SampleGender_test02_out.tsv", TESTDATA("data_out/SampleGender_test02_out.tsv"));
	}

	void method_sry_batch()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " " + TESTDATA("../cppNGS-TEST/data_in/sry.bam") + " -method sry -out out/SampleGender_test03_out.tsv");
		COMPARE_FILES("out/SampleGender_test03_out.tsv", TESTDATA("data_out/SampleGender_test03_out.tsv"));
	}
};

