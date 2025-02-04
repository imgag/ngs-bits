#include "TestFramework.h"

TEST_CLASS(TsvSlice_Test)
{
Q_OBJECT
private slots:
	
	//test with column names
	void test_column_names()
	{
		EXECUTE("TsvSlice", "-cols chr,start,end,ref,obs,snp_q,variant_frequency,sample -in " + TESTDATA("data_in/TsvSlice_in1.tsv") + " -out out/TsvSlice_out1.tsv");
		COMPARE_FILES("out/TsvSlice_out1.tsv", TESTDATA("data_out/TsvSlice_out1.tsv"));
	}

	//test with column numbers
	void test_column_number()
	{
		EXECUTE("TsvSlice", "-numeric -cols 1,2,3,4,5,7,11,22 -in " + TESTDATA("data_in/TsvSlice_in1.tsv") + " -out out/TsvSlice_out1.tsv");
		COMPARE_FILES("out/TsvSlice_out1.tsv", TESTDATA("data_out/TsvSlice_out1.tsv"));
	}

	//test for rm parameter
	void test_rm()
	{
		EXECUTE("TsvSlice", "-rm -cols snp_q,variant_frequency -in " + TESTDATA("data_in/TsvSlice_in1.tsv") + " -out out/TsvSlice_out2.tsv");
		COMPARE_FILES("out/TsvSlice_out2.tsv", TESTDATA("data_out/TsvSlice_out2.tsv"));
	}
};


