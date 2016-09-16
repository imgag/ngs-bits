#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSD_Test)
{
Q_OBJECT
private slots:

	//Normally, one member is tested in one QT slot.
	//Because initializing the database takes very long, all NGSD functionality is tested in one slot.
	void main_tests()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));

		//getProcessingSystem
		QString sys = db.getProcessingSystem("NA12878_03", NGSD::SHORT);
		S_EQUAL(sys, "hpHBOCv5");

		//geneToApproved
		auto gene_app = db.geneToApproved("BRCA1");
		S_EQUAL(gene_app.first, "BRCA1");
		S_EQUAL(gene_app.second, "KEPT: BRCA1 is an approved symbol");
		gene_app = db.geneToApproved("BLABLA");
		S_EQUAL(gene_app.first, "BLABLA");
		S_EQUAL(gene_app.second, "ERROR: BLABLA is unknown symbol");

		//geneToApprovedID
		int gene_app_id = db.geneToApprovedID("BRCA1");
		I_EQUAL(gene_app_id, 1);
		gene_app_id = db.geneToApprovedID("BLABLA");
		I_EQUAL(gene_app_id, -1);
	}

	//Test for debugging (without initialization because of speed)
	/*
	void debug()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");
		NGSD db(true);

		//getProcessingSystem
		QString sys = db.getProcessingSystem("NA12878_03", NGSD::SHORT);
		S_EQUAL(sys, "hpHBOCv5");
	}
	*/
};
