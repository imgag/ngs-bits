#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(BedGeneOverlap_Test)
{
Q_OBJECT
private slots:
	
	void source_ccds()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedGeneOverlap_init.sql"));

		EXECUTE("BedGeneOverlap", "-test -source ccds -in " + TESTDATA("data_in/BedGeneOverlap_in1.bed") + " -out out/BedGeneOverlap_out1.tsv");
		COMPARE_FILES("out/BedGeneOverlap_out1.tsv", TESTDATA("data_out/BedGeneOverlap_out1.tsv"));
	}

	void source_ensembl()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedGeneOverlap_init.sql"));

		EXECUTE("BedGeneOverlap", "-test -source ensembl -in " + TESTDATA("data_in/BedGeneOverlap_in1.bed") + " -out out/BedGeneOverlap_out2.tsv");
		COMPARE_FILES("out/BedGeneOverlap_out2.tsv", TESTDATA("data_out/BedGeneOverlap_out2.tsv"));
	}

};
