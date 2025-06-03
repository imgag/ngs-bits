#include "TestFramework.h"

TEST_CLASS(SampleGender_Test)
{
Q_OBJECT
private slots:
	
	void method_xy()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method xy -build hg19 -out out/SampleGender_test01_out.tsv");
		COMPARE_FILES("out/SampleGender_test01_out.tsv", TESTDATA("data_out/SampleGender_test01_out.tsv"));
	}

	void method_hetx()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -method hetx -build hg19 -out out/SampleGender_test02_out.tsv");
		COMPARE_FILES("out/SampleGender_test02_out.tsv", TESTDATA("data_out/SampleGender_test02_out.tsv"));
	}

	void method_sry_batch()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " " + TESTDATA("../cppNGS-TEST/data_in/sry.bam") + " -method sry -build hg19 -out out/SampleGender_test03_out.tsv");
		COMPARE_FILES("out/SampleGender_test03_out.tsv", TESTDATA("data_out/SampleGender_test03_out.tsv"));
	}

	void method_xy_longread1()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("data_in/SampleGender_in_lr1.bam") + " -method xy -out out/SampleGender_test04_out.tsv -long_read");
		COMPARE_FILES("out/SampleGender_test04_out.tsv", TESTDATA("data_out/SampleGender_test04_out.tsv"));
	}

	void method_xy_longread2()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("data_in/SampleGender_in_lr2.bam") + " -method xy -out out/SampleGender_test05_out.tsv -long_read");
		COMPARE_FILES("out/SampleGender_test05_out.tsv", TESTDATA("data_out/SampleGender_test05_out.tsv"));
	}

	void method_hetx_longread1()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("data_in/SampleGender_in_lr1.bam") + " -method hetx -out out/SampleGender_test06_out.tsv -long_read");
		COMPARE_FILES("out/SampleGender_test06_out.tsv", TESTDATA("data_out/SampleGender_test06_out.tsv"));
	}

	void method_hetx_longread2()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("data_in/SampleGender_in_lr2.bam") + " -method hetx -out out/SampleGender_test07_out.tsv -long_read");
		COMPARE_FILES("out/SampleGender_test07_out.tsv", TESTDATA("data_out/SampleGender_test07_out.tsv"));
	}

	void method_sry_batch_longread()
	{
		EXECUTE("SampleGender", "-in " + TESTDATA("data_in/SampleGender_in_lr1.bam") + " " + TESTDATA("data_in/SampleGender_in_lr2.bam") + " -method sry -out out/SampleGender_test08_out.tsv -long_read");
		COMPARE_FILES("out/SampleGender_test08_out.tsv", TESTDATA("data_out/SampleGender_test08_out.tsv"));
	}

};

