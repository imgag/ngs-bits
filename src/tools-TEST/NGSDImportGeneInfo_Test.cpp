#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportGeneInfo_Test)
{
private:
	
	TEST_METHOD(gnomad_210)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGeneInfo_init.sql"));

		//test
		EXECUTE("NGSDImportGeneInfo", "-test -constraint " + TESTDATA("data_in/NGSDImportGeneInfo_constraint_2.1.txt"));

		//check
		GeneInfo ginfo = db.geneInfo("BRCA1");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.94);
		F_EQUAL(ginfo.oe_mis.toDouble(), 0.92);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.72);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("BRCA2");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.96);
		F_EQUAL(ginfo.oe_mis.toDouble(), 1.07);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.50);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("OR4F5");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.98);
		F_EQUAL(ginfo.oe_mis.toDouble(), 0.83);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.87);
		S_EQUAL(ginfo.inheritance, "AR");

		ginfo = db.geneInfo("DIRC1");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.95);
		F_EQUAL(ginfo.oe_mis.toDouble(), 1.05);
		S_EQUAL(ginfo.oe_lof, "n/a");
	}


	TEST_METHOD(gnomad_211)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGeneInfo_init.sql"));

		//test
		EXECUTE("NGSDImportGeneInfo", "-test -constraint " + TESTDATA("data_in/NGSDImportGeneInfo_constraint_2.1.1.txt"));

		//check
		GeneInfo ginfo = db.geneInfo("BRCA1");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.96);
		F_EQUAL(ginfo.oe_mis.toDouble(), 0.95);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.73);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("BRCA2");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.99);
		F_EQUAL(ginfo.oe_mis.toDouble(), 1.09);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.51);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("OR4F5");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.92);
		F_EQUAL(ginfo.oe_mis.toDouble(), 0.81);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.86);
		S_EQUAL(ginfo.inheritance, "AR");

		ginfo = db.geneInfo("DIRC1");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.95);
		F_EQUAL(ginfo.oe_mis.toDouble(), 1.03);
		S_EQUAL(ginfo.oe_lof, "n/a");
		S_EQUAL(ginfo.inheritance, "n/a");
	}

};


