#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportGeneInfo_Test)
{
private:

	TEST_METHOD(gnomad_411)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGeneInfo_init.sql"));

		//test
		EXECUTE("NGSDImportGeneInfo", "-test -constraint " + TESTDATA("data_in/NGSDImportGeneInfo_gnomad.v4.1.1.constraint_metrics.tsv"));

		//check
		GeneInfo ginfo = db.geneInfo("BRCA1");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.88);
		F_EQUAL(ginfo.oe_mis.toDouble(), 0.91);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.81);
		F_EQUAL(ginfo.pli.toDouble(), 0.00);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("BRCA2");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.96);
		F_EQUAL(ginfo.oe_mis.toDouble(), 1.00);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.73);
		F_EQUAL(ginfo.pli.toDouble(), 0.00);
		S_EQUAL(ginfo.inheritance, "AD");

		ginfo = db.geneInfo("OR4F5");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.28);
		F_EQUAL(ginfo.oe_mis.toDouble(), 0.27);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.17);
		F_EQUAL(ginfo.pli.toDouble(), 0.72);
		S_EQUAL(ginfo.inheritance, "AR");

		ginfo = db.geneInfo("WDR45");
		F_EQUAL(ginfo.oe_syn.toDouble(), 0.83);
		F_EQUAL(ginfo.oe_mis.toDouble(), 0.56);
		F_EQUAL(ginfo.oe_lof.toDouble(), 0.07);
		F_EQUAL(ginfo.pli.toDouble(), 1.00);
		S_EQUAL(ginfo.inheritance, "n/a");

		//check imported version
		QString version = db.getValue("SELECT version FROM db_import_info WHERE name='gnomAD constraints'").toString();
		S_EQUAL(version, "4.1.1");
	}

};


