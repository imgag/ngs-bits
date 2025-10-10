#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(PhenotypeSubtree_Test)
{
private:
	
	TEST_METHOD(one_level)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypeSubtree_init.sql"));

		//test
		EXECUTE("PhenotypeSubtree", "-test -in HP:0001417 -out out/PhenotypeSubtree_out1.tsv");
		COMPARE_FILES("out/PhenotypeSubtree_out1.tsv", TESTDATA("data_out/PhenotypeSubtree_out1.tsv"));
	}

	TEST_METHOD(two_levels)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypeSubtree_init.sql"));

		//test
		EXECUTE("PhenotypeSubtree", "-test -in HP:0000005 -out out/PhenotypeSubtree_out2.tsv");
		COMPARE_FILES("out/PhenotypeSubtree_out2.tsv", TESTDATA("data_out/PhenotypeSubtree_out2.tsv"));
	}
};
