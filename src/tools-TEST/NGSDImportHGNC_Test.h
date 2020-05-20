#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportHGNC_Test)
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
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHGNC_init.sql"));

		//test
		EXECUTE("NGSDImportHGNC", "-test -in " + TESTDATA("data_in/NGSDImportHGNC_in1.txt"));

		//check counts
		int gene_count = db.getValue("SELECT count(*) FROM gene").toInt();
		I_EQUAL(gene_count, 8)
		int alias_count = db.getValue("SELECT count(*) FROM gene_alias").toInt();
		I_EQUAL(alias_count, 39)

		//check TP53
		int gene_id = db.getValue("SELECT id FROM gene WHERE symbol='TP53'").toInt();
		GeneSet previous = db.previousSymbols(gene_id);
		I_EQUAL(previous.count(), 0)
		GeneSet alias = db.synonymousSymbols(gene_id);
		I_EQUAL(alias.count(), 2)
		S_EQUAL(alias[0], "LFS1")
		S_EQUAL(alias[1], "P53")
		GeneInfo gene_info = db.geneInfo("TP53");
		S_EQUAL(gene_info.name, "tumor protein p53")
		QByteArray hgnc_id = db.geneHgncId(gene_id);
		S_EQUAL(hgnc_id, "11998")
		QByteArray type = db.getValue("SELECT type FROM gene WHERE symbol='TP53'").toByteArray();
		S_EQUAL(type, "protein-coding gene")

		//check CA8
		gene_id = db.getValue("SELECT id FROM gene WHERE symbol='CA8'").toInt();
		previous = db.previousSymbols(gene_id);
		I_EQUAL(previous.count(), 1)
		S_EQUAL(previous[0], "CALS")
		alias = db.synonymousSymbols(gene_id);
		I_EQUAL(alias.count(), 1)
		S_EQUAL(alias[0], "CARP")
	}

};

