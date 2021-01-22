#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"
#include <QThread>

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
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3") + " -pseudogenes " + TESTDATA("data_in/NGSDImportEnsembl_in_pseudogenes.txt"));

		//check transcripts
		int count = db.getValue("SELECT count(*) FROM gene_transcript").toInt();
		I_EQUAL(count, 12);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ensembl'").toInt();
		I_EQUAL(count, 8);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ccds'").toInt();
		I_EQUAL(count, 4);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE start_coding IS NULL AND end_coding IS NULL").toInt();
		I_EQUAL(count, 2);

		//check exons
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt, gene g WHERE g.id=gt.gene_id AND ge.transcript_id=gt.id AND g.symbol='DDX11L1'").toInt();
		I_EQUAL(count, 3);
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt WHERE ge.transcript_id=gt.id AND gt.name='CCDS9344.1'").toInt();
		I_EQUAL(count, 26);


		//check pseudogenes
		int parent_gene_id = db.geneToApprovedID("ABCD1");
		int pseudogene_id = db.getValue("SELECT pseudogene_gene_id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toInt();
		QString pseudogene = db.geneSymbol(pseudogene_id);
		S_EQUAL(pseudogene, "ABCD1P2");

		parent_gene_id = db.geneToApprovedID("AARS1");
		QString gene_name = db.getValue("SELECT gene_name FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toString();
		S_EQUAL(gene_name, "ENSG00000249038;RP11-149A7");

		pseudogene_id = db.geneToApprovedID("ABCD1P2");
		parent_gene_id = db.getValue("SELECT parent_gene_id FROM gene_pseudogene_relation WHERE pseudogene_gene_id=" + QByteArray::number(pseudogene_id)).toInt();
		QString parent_gene = db.geneSymbol(parent_gene_id);
		S_EQUAL(parent_gene, "ABCD1");
	}

};


