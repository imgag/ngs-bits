#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportUCSC_Test)
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
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportUCSC_init.sql"));

		//test
		EXECUTE("NGSDImportUCSC", "-test -refGene " + TESTDATA("data_in/NGSDImportUCSC_refGene.txt") + " -kg " + TESTDATA("data_in/NGSDImportUCSC_kg.txt") + " -kgXR " + TESTDATA("data_in/NGSDImportUCSC_kgXR.txt") + " -ccdsKM " + TESTDATA("data_in/NGSDImportUCSC_ccdsKM.txt") + " -ccds " + TESTDATA("data_in/NGSDImportUCSC_ccds.txt"));

		//check
		int transcript_count = db.getValue("SELECT count(*) FROM gene_transcript").toInt();
		I_EQUAL(transcript_count, 10)
		int exon_count = db.getValue("SELECT count(*) FROM gene_exon").toInt();
		I_EQUAL(exon_count, 106)
		int non_coding_count = db.getValue("SELECT count(*) FROM gene_transcript WHERE start_coding IS NULL AND end_coding IS NULL").toInt();
		I_EQUAL(non_coding_count, 4)
	}

};


