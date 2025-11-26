#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(BedToEpigen_Test)
{
private:

	TEST_METHOD(gz_input)
	{
		EXECUTE("BedToEpigen", "-sample HG002_01 -id_file " + TESTDATA("data_in/BedToEpigen_in_ids.csv") + " -in " + TESTDATA("data_in/BedToEpigen_in1.bed.gz") + " -out out/BedToEpigen_out1.tsv");
		COMPARE_FILES("out/BedToEpigen_out1.tsv", TESTDATA("data_out/BedToEpigen_out1.tsv"));
	}

	TEST_METHOD(unzipped_input)
	{
		EXECUTE("BedToEpigen", "-sample HG002_01 -id_file " + TESTDATA("data_in/BedToEpigen_in_ids.csv") + " -in " + TESTDATA("data_in/BedToEpigen_in2.bed") + " -out out/BedToEpigen_out2.tsv");
		COMPARE_FILES("out/BedToEpigen_out2.tsv", TESTDATA("data_out/BedToEpigen_out1.tsv"));
	}

};
