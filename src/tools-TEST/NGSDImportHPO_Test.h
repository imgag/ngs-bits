#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportHPO_Test)
{
Q_OBJECT
private slots:
	
	/*
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
	*/

	void with_hpophen()
	{
		//TODO find way to verify by hand!
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -hpophen " + TESTDATA("data_in/NGSDImportHPO_phenotype.hpoa") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 14)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 129);
		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE details LIKE '%HPO%'").toInt();
		I_EQUAL(count, 129);
		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE evidence !='n/a'").toInt();
		I_EQUAL(count, 92);

		QStringList results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%PCS%'");
		I_EQUAL(results.length(), 2);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "high")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%TAS%'");
		I_EQUAL(results.length(), 75);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "medium")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%IEA%'");
		I_EQUAL(results.length(), 15);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "low")
		}
	}

//	void with_gencc()
//	{
//		QString host = Settings::string("ngsd_test_host", true);
//		if (host=="") SKIP("Test needs access to the NGSD test database!");

//		//init
//		NGSD db(true);
//		db.init();
//		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

//		//test
//		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -gencc " + TESTDATA("data_in/NGSDImportHPO_gencc.csv") + " -debug");

//		//check
//		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
//		I_EQUAL(count, 14)
//		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
//		I_EQUAL(count, 11)
//		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
//		I_EQUAL(count, 127);
//		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE details LIKE '%GenCC%'").toInt();
//		I_EQUAL(count, 35);

//		QStringList results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Animal%'");
//		I_EQUAL(results.length(), 2);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "low")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Limited%'");
//		I_EQUAL(results.length(), 2);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "low")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Supportive%'");
//		I_EQUAL(results.length(), 2);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "low")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Moderate%'");
//		I_EQUAL(results.length(), 2);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "medium")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Strong%'");
//		I_EQUAL(results.length(), 16);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "high")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Definitive%'");
//		I_EQUAL(results.length(), 11);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "high")
//		}

//	}

//	void with_decipher()
//	{
//		//Verified by hand
//		QString host = Settings::string("ngsd_test_host", true);
//		if (host=="") SKIP("Test needs access to the NGSD test database!");

//		//init
//		NGSD db(true);
//		db.init();
//		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

//		//test
//		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -decipher " + TESTDATA("data_in/NGSDImportHPO_decipher.csv") + " -debug");

//		//check

//		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
//		I_EQUAL(count, 14)
//		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
//		I_EQUAL(count, 11)
//		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
//		I_EQUAL(count, 127);
//		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE details LIKE '%Decipher%'").toInt();
//		I_EQUAL(count, 17);

//		QStringList results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%both RD and IF%'");
//		I_EQUAL(results.length(), 3);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "low")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%possible%'");
//		I_EQUAL(results.length(), 1);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "low")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%probable%'");
//		I_EQUAL(results.length(), 1);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "medium")
//		}

//		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%confirmed%'");
//		I_EQUAL(results.length(), 12);
//		foreach (const QString res, results)
//		{
//			S_EQUAL(res, "high")
//		}

//	}
};

