#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(BedGeneOverlap_Test)
{
private:
	
	TEST_METHOD(source_ccds)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedGeneOverlap_init.sql"));

		EXECUTE("BedGeneOverlap", "-test -source ccds -in " + TESTDATA("data_in/BedGeneOverlap_in1.bed") + " -out out/BedGeneOverlap_out1.tsv");
		COMPARE_FILES("out/BedGeneOverlap_out1.tsv", TESTDATA("data_out/BedGeneOverlap_out1.tsv"));
	}

	TEST_METHOD(source_ensembl)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedGeneOverlap_init.sql"));

		EXECUTE("BedGeneOverlap", "-test -source ensembl -in " + TESTDATA("data_in/BedGeneOverlap_in1.bed") + " -out out/BedGeneOverlap_out2.tsv");
		COMPARE_FILES("out/BedGeneOverlap_out2.tsv", TESTDATA("data_out/BedGeneOverlap_out2.tsv"));
	}

};
