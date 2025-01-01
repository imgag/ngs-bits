#include "TestFramework.h"

TEST_CLASS(TsvDiff_Test)
{
Q_OBJECT
private slots:
	
	void no_difference()
	{
		EXECUTE("TsvDiff", "-in1 " + TESTDATA("data_in/TsvDiff_in1.tsv") + " -in2 " + TESTDATA("data_in/TsvDiff_in1.tsv") + " -out out/TsvDiff_out1.txt");
		COMPARE_FILES("out/TsvDiff_out1.txt", TESTDATA("data_out/TsvDiff_out1.txt"));
	}

	void differences_in_header_and_content_lines()
	{
		EXECUTE("TsvDiff", "-in1 " + TESTDATA("data_in/TsvDiff_in1.tsv") + " -in2 " + TESTDATA("data_in/TsvDiff_in2.tsv") + " -out out/TsvDiff_out2.txt -no_error");
		COMPARE_FILES("out/TsvDiff_out2.txt", TESTDATA("data_out/TsvDiff_out2.txt"));
	}

	void skip_comments_matching()
	{
		EXECUTE("TsvDiff", "-in1 " + TESTDATA("data_in/TsvDiff_in1.tsv") + " -in2 " + TESTDATA("data_in/TsvDiff_in2.tsv") + " -out out/TsvDiff_out3.txt -skip_comments_matching insert1,bla -no_error");
		COMPARE_FILES("out/TsvDiff_out3.txt", TESTDATA("data_out/TsvDiff_out3.txt"));
	}

	void comp()
	{
		EXECUTE("TsvDiff", "-in1 " + TESTDATA("data_in/TsvDiff_in1.tsv") + " -in2 " + TESTDATA("data_in/TsvDiff_in2.tsv") + " -out out/TsvDiff_out4.txt -comp chr,start,end,ref,obs -no_error");
		COMPARE_FILES("out/TsvDiff_out4.txt", TESTDATA("data_out/TsvDiff_out4.txt"));
	}

	void skip_cols()
	{
		EXECUTE("TsvDiff", "-in1 " + TESTDATA("data_in/TsvDiff_in1.tsv") + " -in2 " + TESTDATA("data_in/TsvDiff_in3.tsv") + " -out out/TsvDiff_out5.txt -skip_cols snp_q,depth -no_error");
		COMPARE_FILES("out/TsvDiff_out5.txt", TESTDATA("data_out/TsvDiff_out5.txt"));
	}

	void diff_abs()
	{
		EXECUTE("TsvDiff", "-in1 " + TESTDATA("data_in/TsvDiff_in1.tsv") + " -in2 " + TESTDATA("data_in/TsvDiff_in2.tsv") + " -out out/TsvDiff_out6.txt -diff_abs depth=1,snp_q=3 -no_error");
		COMPARE_FILES("out/TsvDiff_out6.txt", TESTDATA("data_out/TsvDiff_out6.txt"));
	}

};


