#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(PhenotypeSubtree_Test)
{
Q_OBJECT
private slots:
	
	void one_level()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypeSubtree_init.sql"));

		//test
		EXECUTE("PhenotypeSubtree", "-test -in HP:0001417 -out out/PhenotypeSubtree_out1.tsv");
		COMPARE_FILES("out/PhenotypeSubtree_out1.tsv", TESTDATA("data_out/PhenotypeSubtree_out1.tsv"));
	}

	void two_levels()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/PhenotypeSubtree_init.sql"));

		//test
		EXECUTE("PhenotypeSubtree", "-test -in HP:0000005 -out out/PhenotypeSubtree_out2.tsv");
		COMPARE_FILES("out/PhenotypeSubtree_out2.tsv", TESTDATA("data_out/PhenotypeSubtree_out2.tsv"));
	}
};
