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
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -ignore_indels -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line32.log", TESTDATA("data_out/BamClipOverlap_out4.log"));
	}

	void test_05()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_baseq -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line39.log", TESTDATA("data_out/BamClipOverlap_out5.log"));
	}

	void test_06()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_mapq -ignore_indels -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line46.log", TESTDATA("data_out/BamClipOverlap_out6.log"));
	}

	void test_07()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_remove -ignore_indels -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line53.log", TESTDATA("data_out/BamClipOverlap_out7.log"));
	}

	void test_08()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line60.log", TESTDATA("data_out/BamClipOverlap_out8.log"));
	}

	void test_09()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_basen -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line67.log", TESTDATA("data_out/BamClipOverlap_out9.log"));
	}

	void indel_in_overlap()
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in5.bam") + " -out out/BamClipOverlap_out5.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES("out/BamClipOverlap_Test_line74.log", TESTDATA("data_out/BamClipOverlap_out10.log"));
	}
};

