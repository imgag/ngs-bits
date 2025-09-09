#include "TestFramework.h"


TEST_CLASS(BamClipOverlap_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in1.bam") + " -out out/BamClipOverlap_out1.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out1.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out1.log"));
	}

	TEST_METHOD(test_02)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in2.bam") + " -out out/BamClipOverlap_out2.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out2.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out2.log"));
	}

	TEST_METHOD(test_03)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in3.bam") + " -out out/BamClipOverlap_out3.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out3.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out3.log"));
	}

	TEST_METHOD(test_04)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -ignore_indels -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out4.log"));
	}

	TEST_METHOD(test_05)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_baseq -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out5.log"));
	}

	TEST_METHOD(test_06)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_mapq -ignore_indels -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out6.log"));
	}

	TEST_METHOD(test_07)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_remove -ignore_indels -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out7.log"));
	}

	TEST_METHOD(test_08)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out8.log"));
	}

	TEST_METHOD(test_09)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in4.bam") + " -out out/BamClipOverlap_out4.bam -overlap_mismatch_basen -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out4.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out9.log"));
	}

	TEST_METHOD(indel_in_overlap)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("data_in/BamClipOverlap_in5.bam") + " -out out/BamClipOverlap_out5.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out5.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out10.log"));
	}

	TEST_METHOD(cigar_with_only_insertion)
	{
		EXECUTE("BamClipOverlap", "-in " + TESTDATA("../cppNGS-TEST/data_in/BamReader_insert_only.bam") + " -out out/BamClipOverlap_out6.bam -v");
		IS_TRUE(QFile::exists("out/BamClipOverlap_out6.bam"));
		COMPARE_FILES(lastLogFile(), TESTDATA("data_out/BamClipOverlap_out11.log"));
	}
};

