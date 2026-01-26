#include "TestFramework.h"

TEST_CLASS(TsvFilter_Test)
{
private:
	
	//test >=
	TEST_METHOD(test_01)
	{
		EXECUTE("TsvFilter", "-filter depth%20>=%20205 -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out1.tsv");
		COMPARE_FILES("out/TsvFilter_out1.tsv", TESTDATA("data_out/TsvFilter_out1.tsv"));
	}

	//test >
	TEST_METHOD(test_02)
	{
		EXECUTE("TsvFilter", "-filter depth%20>%20205 -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out2.tsv");
		COMPARE_FILES("out/TsvFilter_out2.tsv", TESTDATA("data_out/TsvFilter_out2.tsv"));
	}

	//test =
	TEST_METHOD(test_03)
	{
		EXECUTE("TsvFilter", "-filter depth%20=%20205 -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out3.tsv");
		COMPARE_FILES("out/TsvFilter_out3.tsv", TESTDATA("data_out/TsvFilter_out3.tsv"));
	}

	//test <
	TEST_METHOD(test_04)
	{
		EXECUTE("TsvFilter", "-filter snp_q%20>%20186 -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out4.tsv");
		COMPARE_FILES("out/TsvFilter_out4.tsv", TESTDATA("data_out/TsvFilter_out4.tsv"));
	}

	//test <=
	TEST_METHOD(test_05)
	{
		EXECUTE("TsvFilter", "-filter snp_q%20>=%20186 -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out5.tsv");
		COMPARE_FILES("out/TsvFilter_out5.tsv", TESTDATA("data_out/TsvFilter_out5.tsv"));
	}

	//test is
	TEST_METHOD(test_06)
	{
		EXECUTE("TsvFilter", "-filter ref%20is%20- -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out6.tsv");
		COMPARE_FILES("out/TsvFilter_out6.tsv", TESTDATA("data_out/TsvFilter_out6.tsv"));
	}

	//test contains
	TEST_METHOD(test_07)
	{
		EXECUTE("TsvFilter", "-filter obs%20contains%20CT -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out7.tsv");
		COMPARE_FILES("out/TsvFilter_out7.tsv", TESTDATA("data_out/TsvFilter_out7.tsv"));
	}

	//test contains - inverted
	TEST_METHOD(test_08)
	{
		EXECUTE("TsvFilter", "-filter genotype%20is%20hom -v -in " + TESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out8.tsv");
		COMPARE_FILES("out/TsvFilter_out8.tsv", TESTDATA("data_out/TsvFilter_out8.tsv"));
	}

};


