#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(BedAnnotateGenes_Test)
{
private:

	TEST_METHOD(without_existing_annotations)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedAnnotateGenes_init.sql"));

		//test
		EXECUTE("BedAnnotateGenes", "-test -in " + TESTDATA("data_in/BedAnnotateGenes_in1.bed") + " -out out/BedAnnotateGenes_out1.bed");
		COMPARE_FILES("out/BedAnnotateGenes_out1.bed", TESTDATA("data_out/BedAnnotateGenes_out1.bed"));
	}

	TEST_METHOD(with_existing_annotations_and_extend25)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedAnnotateGenes_init.sql"));

		//test
		EXECUTE("BedAnnotateGenes", "-test -extend 25 -in " + TESTDATA("data_in/BedAnnotateGenes_in2.bed") + " -out out/BedAnnotateGenes_out2.bed");
		COMPARE_FILES("out/BedAnnotateGenes_out2.bed", TESTDATA("data_out/BedAnnotateGenes_out2.bed"));
	}


	TEST_METHOD(with_existing_annotations_and_clear)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/BedAnnotateGenes_init.sql"));

		//test
		EXECUTE("BedAnnotateGenes", "-test -clear -in " + TESTDATA("data_in/BedAnnotateGenes_in2.bed") + " -out out/BedAnnotateGenes_out3.bed");
		COMPARE_FILES("out/BedAnnotateGenes_out3.bed", TESTDATA("data_out/BedAnnotateGenes_out3.bed"));
	}
};
