#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportORPHA_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportORPHA_init.sql"));

		//test
		EXECUTE("NGSDImportORPHA", "-test -terms " + TESTDATA("data_in/NGSDImportORPHA_terms.xml") + " -genes " + TESTDATA("data_in/NGSDImportORPHA_genes.xml"));

		//check
		I_EQUAL(db.getValue("SELECT count(*) FROM disease_term").toInt(), 3)
		I_EQUAL(db.getValue("SELECT count(*) FROM disease_gene").toInt(), 4)
	}
};

