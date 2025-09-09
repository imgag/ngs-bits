#include "TestFramework.h"

TEST_CLASS(BedIntersect_Test)
{
private:
	
	TEST_METHOD(mode_intersect)
	{
		EXECUTE("BedIntersect", "-in " + TESTDATA("data_in/exome.bed") + " -in2 " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test01_out.bed");
		COMPARE_FILES("out/BedIntersect_test01_out.bed", TESTDATA("data_out/BedIntersect_test01_out.bed"));
	}
	
	TEST_METHOD(mode_in)
	{
		EXECUTE("BedIntersect", "-in " + TESTDATA("data_in/exome.bed") + " -in2 " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test02_out.bed -mode in");
		COMPARE_FILES("out/BedIntersect_test02_out.bed", TESTDATA("data_out/BedIntersect_test02_out.bed"));
	}
	
	TEST_METHOD(mode_in2)
	{
		EXECUTE("BedIntersect", "-in " + TESTDATA("data_in/exome.bed") + " -in2 " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/BedIntersect_test03_out.bed -mode in2");
		COMPARE_FILES("out/BedIntersect_test03_out.bed", TESTDATA("data_out/BedIntersect_test03_out.bed"));
	}

};
