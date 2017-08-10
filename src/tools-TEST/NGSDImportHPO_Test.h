#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportHPO_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -genes " + TESTDATA("data_in/NGSDImportHPO_genes.txt"));

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 6)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 5)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 52)
	}

};

