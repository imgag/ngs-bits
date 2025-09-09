#include "TestFramework.h"

TEST_CLASS(BedSort_Test)
{
private:
	
	TEST_METHOD(deafult_parameters)
	{
		EXECUTE("BedSort", "-in " + TESTDATA("data_in/exome.bed") + " -out out/BedSort_test01_out.bed");
		COMPARE_FILES("out/BedSort_test01_out.bed", TESTDATA("data_out/BedSort_test01_out.bed"));
	}

	TEST_METHOD(unique)
	{
		EXECUTE("BedSort", "-in " + TESTDATA("data_in/BedSort_in2.bed") + " -out out/BedSort_test02_out.bed -uniq");
		COMPARE_FILES("out/BedSort_test02_out.bed", TESTDATA("data_out/BedSort_test02_out.bed"));
	}

	TEST_METHOD(with_name)
	{
		EXECUTE("BedSort", "-in " + TESTDATA("data_in/BedSort_in1.bed") + " -out out/BedSort_test03_out.bed -with_name");
		COMPARE_FILES("out/BedSort_test03_out.bed", TESTDATA("data_out/BedSort_test03_out.bed"));
	}

};
