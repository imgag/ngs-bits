#include "TestFramework.h"

TEST_CLASS(FastaInfo_Test)
{
Q_OBJECT
private slots:
	
	void basic_test()
	{
		EXECUTE("FastaInfo", "-in " + TESTDATA("data_in/FastaInfo_in1.fa") + " -out out/FastaInfo_test01_out.txt -write_n out/FastaInfo_test01_out_n.bed -write_other out/FastaInfo_test01_out_other.bed");
		COMPARE_FILES("out/FastaInfo_test01_out.txt", TESTDATA("data_out/FastaInfo_test01_out.txt"));
		COMPARE_FILES("out/FastaInfo_test01_out_n.bed", TESTDATA("data_out/FastaInfo_test01_out_n.bed"));
		COMPARE_FILES("out/FastaInfo_test01_out_other.bed", TESTDATA("data_out/FastaInfo_test01_out_other.bed"));
	}

};
