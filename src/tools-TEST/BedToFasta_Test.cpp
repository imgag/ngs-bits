#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedToFasta_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
	
		EXECUTE("BedToFasta", "-in " + TESTDATA("data_in/BedToFasta_in1.bed") + " -out out/BedToFasta_test01_out.bed -ref " + ref_file);
		COMPARE_FILES("out/BedToFasta_test01_out.bed", TESTDATA("data_out/BedToFasta_out1.fa"));
	}

};
