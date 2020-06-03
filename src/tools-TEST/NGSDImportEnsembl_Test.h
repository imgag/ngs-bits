#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportEnsembl_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportEnsembl_init.sql"));

		//test
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3"));

		//check transcripts
		int count = db.getValue("SELECT count(*) FROM gene_transcript").toInt();
		I_EQUAL(count, 6)
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ensembl'").toInt();
		I_EQUAL(count, 4)
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ccds'").toInt();
		I_EQUAL(count, 2)
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE start_coding IS NULL AND end_coding IS NULL").toInt();
		I_EQUAL(count, 1)

		//check exons
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt, gene g WHERE g.id=gt.gene_id AND ge.transcript_id=gt.id AND g.symbol='DDX11L1'").toInt();
		I_EQUAL(count, 3)
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt WHERE ge.transcript_id=gt.id AND gt.name='CCDS9344.1'").toInt();
		I_EQUAL(count, 26)
	}

};


