#include "TestFramework.h"

TEST_CLASS(BedInfo_Test)
{
private:
	
	TEST_METHOD(base_test)
	{
		EXECUTE("BedInfo", "-in " + TESTDATA("data_in/exome.bed") + " -out out/BedInfo_test01_out.txt");
		COMPARE_FILES("out/BedInfo_test01_out.txt", TESTDATA("data_out/BedInfo_test01_out.txt"));
	}
	
	TEST_METHOD(with_parameter_fai)
	{
		EXECUTE("BedInfo", "-in " + TESTDATA("data_in/BedInfo_in2.bed") + " -out out/BedInfo_test02_out.txt -nomerge -fai " + TESTDATA("data_in/hg19.fa.fai"));
		COMPARE_FILES("out/BedInfo_test02_out.txt", TESTDATA("data_out/BedInfo_test02_out.txt"));
	}

	TEST_METHOD(with_parameter_filename)
	{
		EXECUTE("BedInfo", "-in " + TESTDATA("data_in/BedInfo_in2.bed") + " -out out/BedInfo_test03_out.txt -filename");
		COMPARE_FILES("out/BedInfo_test03_out.txt", TESTDATA("data_out/BedInfo_test03_out.txt"));
	}

};
