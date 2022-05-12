#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(BedAnnotateGenes_Test)
{
Q_OBJECT
private slots:

	void without_existing_annotations()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedAnnotateGenes_init.sql"));

		//test
		EXECUTE("BedAnnotateGenes", "-test -in " + TESTDATA("data_in/BedAnnotateGenes_in1.bed") + " -out out/BedAnnotateGenes_out1.bed");
		COMPARE_FILES("out/BedAnnotateGenes_out1.bed", TESTDATA("data_out/BedAnnotateGenes_out1.bed"));
	}

	void with_existing_annotations_and_extend25()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedAnnotateGenes_init.sql"));

		//test
		EXECUTE("BedAnnotateGenes", "-test -extend 25 -in " + TESTDATA("data_in/BedAnnotateGenes_in2.bed") + " -out out/BedAnnotateGenes_out2.bed");
		COMPARE_FILES("out/BedAnnotateGenes_out2.bed", TESTDATA("data_out/BedAnnotateGenes_out2.bed"));
	}


	void with_existing_annotations_and_clear()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedAnnotateGenes_init.sql"));

		//test
		EXECUTE("BedAnnotateGenes", "-test -clear -in " + TESTDATA("data_in/BedAnnotateGenes_in2.bed") + " -out out/BedAnnotateGenes_out3.bed");
		COMPARE_FILES("out/BedAnnotateGenes_out3.bed", TESTDATA("data_out/BedAnnotateGenes_out3.bed"));
	}
};
