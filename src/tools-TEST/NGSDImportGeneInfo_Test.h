#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportGeneInfo_Test)
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
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGeneInfo_init.sql"));

		//test
		EXECUTE("NGSDImportGeneInfo", "-test -constraint " + TESTDATA("data_in/NGSDImportGeneInfo_constraint.txt"));

		//check
		GeneInfo ginfo = db.geneInfo("BRCA1");
		F_EQUAL(ginfo.exac_pli.toDouble(), 0.00);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("BRCA2");
		F_EQUAL(ginfo.exac_pli.toDouble(), 0.00);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("OR4F5");
		F_EQUAL(ginfo.exac_pli.toDouble(), 0.18);
		S_EQUAL(ginfo.inheritance, "AR");
	}

};


