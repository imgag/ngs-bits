#include "TestFramework.h"

TEST_CLASS(TsvFilter_Test)
{
Q_OBJECT
private slots:
	
	//test >=
	void test_01()
	{
		TFW_EXEC("TsvFilter", "-filter depth%20>=%20205 -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out1.tsv");
		TFW::comareFiles("out/TsvFilter_out1.tsv", QFINDTESTDATA("data_out/TsvFilter_out1.tsv"));
	}

	//test >
	void test_02()
	{
		TFW_EXEC("TsvFilter", "-filter depth%20>%20205 -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out2.tsv");
		TFW::comareFiles("out/TsvFilter_out2.tsv", QFINDTESTDATA("data_out/TsvFilter_out2.tsv"));
	}

	//test =
	void test_03()
	{
		TFW_EXEC("TsvFilter", "-filter depth%20=%20205 -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out3.tsv");
		TFW::comareFiles("out/TsvFilter_out3.tsv", QFINDTESTDATA("data_out/TsvFilter_out3.tsv"));
	}

	//test <
	void test_04()
	{
		TFW_EXEC("TsvFilter", "-filter snp_q%20>%20186 -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out4.tsv");
		TFW::comareFiles("out/TsvFilter_out4.tsv", QFINDTESTDATA("data_out/TsvFilter_out4.tsv"));
	}

	//test <=
	void test_05()
	{
		TFW_EXEC("TsvFilter", "-filter snp_q%20>=%20186 -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out5.tsv");
		TFW::comareFiles("out/TsvFilter_out5.tsv", QFINDTESTDATA("data_out/TsvFilter_out5.tsv"));
	}

	//test is
	void test_06()
	{
		TFW_EXEC("TsvFilter", "-filter ref%20is%20- -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out6.tsv");
		TFW::comareFiles("out/TsvFilter_out6.tsv", QFINDTESTDATA("data_out/TsvFilter_out6.tsv"));
	}

	//test contains
	void test_07()
	{
		TFW_EXEC("TsvFilter", "-filter obs%20contains%20CT -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out7.tsv");
		TFW::comareFiles("out/TsvFilter_out7.tsv", QFINDTESTDATA("data_out/TsvFilter_out7.tsv"));
	}

	//test contains - inverted
	void test_08()
	{
		TFW_EXEC("TsvFilter", "-filter genotype%20is%20hom -v -in " + QFINDTESTDATA("data_in/TsvFilter_in1.tsv") + " -out out/TsvFilter_out8.tsv");
		TFW::comareFiles("out/TsvFilter_out8.tsv", QFINDTESTDATA("data_out/TsvFilter_out8.tsv"));
	}

};


