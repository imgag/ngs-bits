#include "TestFramework.h"

TEST_CLASS(FastaInfo_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("FastaInfo", "-in " + TESTDATA("data_in/dummy.fa") + " -out out/FastaInfo_test01_out.txt");
		COMPARE_FILES("out/FastaInfo_test01_out.txt", TESTDATA("data_out/FastaInfo_test01_out.txt"));
	}

};
