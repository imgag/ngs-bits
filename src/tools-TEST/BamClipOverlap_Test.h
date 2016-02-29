#include "TestFramework.h"


TEST_CLASS(BamClipOverlap_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in1.bam") + " -out out/BamClipOverlap_out1.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out1.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line11.log", TESTDATA("data_out/BamClipOverlap_out1.log"));
	}

	void test_02()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in2.bam") + " -out out/BamClipOverlap_out2.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out2.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line18.log", TESTDATA("data_out/BamClipOverlap_out2.log"));
	}

	void test_03()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in3.bam") + " -out out/BamClipOverlap_out3.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out3.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line25.log", TESTDATA("data_out/BamClipOverlap_out3.log"));
	}

	void test_04()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line25.log", TESTDATA("data_out/BamClipOverlap_out4.log"));
	}
};

