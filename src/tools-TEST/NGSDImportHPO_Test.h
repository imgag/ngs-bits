#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportHPO_Test)
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
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 14)
		count = db.getValue("SELECT count(*) FROM hpo_term WHERE synonyms!=''").toInt();
		I_EQUAL(count, 3)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 121)
		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE gene='PTEN_ALT'").toInt();
		I_EQUAL(count, 0)
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA1"))
		IS_FALSE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA2"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Autosomal dominant inheritance"), false, false).contains("PTEN"))
	}

	void with_omim()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -omim " + TESTDATA("data_in/NGSDImportHPO_omim.txt"));

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 14)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 149)
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA1"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA2"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Autosomal dominant inheritance"), false, false).contains("PTEN"))
	}

	void with_clinvar()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -clinvar " + TESTDATA("data_in/NGSDImportHPO_clinvar.txt") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 14)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 122)
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA1"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA2"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Autosomal dominant inheritance"), false, false).contains("PTEN"))
	}

	void with_hgmd()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -hgmd " + TESTDATA("data_in/NGSDImportHPO_hgmd.dump") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 14)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 147)
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA1"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA2"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Autosomal dominant inheritance"), false, false).contains("PTEN"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BARD1"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Fibroadenoma of the breast"), false, false).contains("WRN"))
	}

};

