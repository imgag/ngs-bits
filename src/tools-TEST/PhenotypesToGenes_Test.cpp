#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(PhenotypesToGenes_Test)
{
private:

	TEST_METHOD(default_parameters)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypesToGenes_init.sql"));

		//test
		EXECUTE("PhenotypesToGenes", "-test -in " + TESTDATA("data_in/PhenotypesToGenes_in1.txt") + " -ignore_invalid -out out/PhenotypesToGenes_out1.txt");
		COMPARE_FILES("out/PhenotypesToGenes_out1.txt", TESTDATA("data_out/PhenotypesToGenes_out1.txt"));
	}

	TEST_METHOD(filterungSources)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypesToGenes_init.sql"));

		//test
		EXECUTE("PhenotypesToGenes", "-test -in " + TESTDATA("data_in/PhenotypesToGenes_in1.txt") + " -source HPO -ignore_invalid -out out/PhenotypesToGenes_out2.txt");
		COMPARE_FILES("out/PhenotypesToGenes_out2.txt", TESTDATA("data_out/PhenotypesToGenes_out2.txt"));
	}

	TEST_METHOD(filteringEvidences)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypesToGenes_init.sql"));

		//test
		EXECUTE("PhenotypesToGenes", "-test -in " + TESTDATA("data_in/PhenotypesToGenes_in1.txt") + " -evidence high -ignore_invalid -out out/PhenotypesToGenes_out3.txt");
		COMPARE_FILES("out/PhenotypesToGenes_out3.txt", TESTDATA("data_out/PhenotypesToGenes_out3.txt"));
	}

	TEST_METHOD(filteringSourcesAndEvidences)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypesToGenes_init.sql"));

		//test
		EXECUTE("PhenotypesToGenes", "-test -in " + TESTDATA("data_in/PhenotypesToGenes_in1.txt") + " -source GenCC -evidence medium -ignore_invalid -out out/PhenotypesToGenes_out4.txt");
		COMPARE_FILES("out/PhenotypesToGenes_out4.txt", TESTDATA("data_out/PhenotypesToGenes_out4.txt"));
	}
};
