#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateGC_Test)
{
private:

	TEST_METHOD(default_params)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
	
		EXECUTE("BedAnnotateGC", "-in " + TESTDATA("data_in/BedAnnotateGC_in1.bed") + " -out out/BedAnnotateGC_out1.bed -ref " + ref_file);
		COMPARE_FILES("out/BedAnnotateGC_out1.bed", TESTDATA("data_out/BedAnnotateGC_out1.bed"));
	}

	TEST_METHOD(clear_extend20)
	{
		QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("BedAnnotateGC", "-clear -extend 20 -in " + TESTDATA("data_in/BedAnnotateGC_in2.bed") + " -out out/BedAnnotateGC_out2.bed -ref " + ref_file);
		COMPARE_FILES_DELTA("out/BedAnnotateGC_out2.bed", TESTDATA("data_out/BedAnnotateGC_out2.bed"), 1.0, true, '\t'); //delta because of macOS rounding problems
	}

};
