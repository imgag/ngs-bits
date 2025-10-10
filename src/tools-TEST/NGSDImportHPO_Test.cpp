#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportHPO_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
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

	TEST_METHOD(with_omim)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -omim " + TESTDATA("data_in/NGSDImportHPO_omim.txt"));

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 149)
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA1"));
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA2"));
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Autosomal dominant inheritance"), false, false).contains("PTEN"));

		QStringList results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%(1)%'");
		I_EQUAL(results.length(), 4);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "low")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%(2)%'");
		I_EQUAL(results.length(), 4);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "low")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%(3)%'");
		I_EQUAL(results.length(), 61);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "high")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%(4)%'");
		I_EQUAL(results.length(), 4);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "high")
		}
	}

	TEST_METHOD(with_clinvar)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -clinvar " + TESTDATA("data_in/NGSDImportHPO_clinvar.txt") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 122)
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA1"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Breast carcinoma"), false, false).contains("BRCA2"))
		IS_TRUE(db.phenotypeToGenes(db.phenotypeIdByName("Autosomal dominant inheritance"), false, false).contains("PTEN"))
	}

	TEST_METHOD(with_hgmd)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -hgmd " + TESTDATA("data_in/NGSDImportHPO_hgmd.dump") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
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


	TEST_METHOD(with_hpophen)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -hpophen " + TESTDATA("data_in/NGSDImportHPO_phenotype.hpoa") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 129);
		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE details LIKE '%HPO%'").toInt();
		I_EQUAL(count, 129);
		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE evidence !='n/a'").toInt();
		I_EQUAL(count, 57);

		QStringList results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%PCS%'");
		I_EQUAL(results.length(), 2);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "high")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%TAS%'");
		I_EQUAL(results.length(), 44);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "medium")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%IEA%'");
		I_EQUAL(results.length(), 11);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "low")
		}
	}

	TEST_METHOD(with_gencc)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -gencc " + TESTDATA("data_in/NGSDImportHPO_gencc.csv") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 127);
		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE details LIKE '%GenCC%'").toInt();
		I_EQUAL(count, 35);

		QStringList results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Animal%'");
		I_EQUAL(results.length(), 2);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "low")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Limited%'");
		I_EQUAL(results.length(), 2);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "low")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Supportive%'");
		I_EQUAL(results.length(), 2);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "low")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Moderate%'");
		I_EQUAL(results.length(), 2);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "medium")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Strong%'");
		I_EQUAL(results.length(), 16);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "high")
		}

		results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%Definitive%'");
		I_EQUAL(results.length(), 11);
		foreach (const QString res, results)
		{
			S_EQUAL(res, "high")
		}

	}

	TEST_METHOD(with_g2p)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -g2p " + TESTDATA("data_in/NGSDImportHPO_decipher1.csv") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 130);
		count = db.getValue("SELECT count(*) FROM hpo_genes WHERE details LIKE '%G2P%'").toInt();
		I_EQUAL(count, 22);

		QStringList results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%G2P, definitive,%'");
		I_EQUAL(results.length(), 21);
		foreach (QString res, results)
		{
			S_EQUAL(res, "high")
		}

		results = results = db.getValues("SELECT evidence FROM hpo_genes WHERE details LIKE '%G2P, strong,%'");
		I_EQUAL(results.length(), 1);
		foreach (QString res, results)
		{
			S_EQUAL(res, "high")
		}
	}

	TEST_METHOD(with_all)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHPO_init.sql"));

		//test
		EXECUTE("NGSDImportHPO", "-test -obo " + TESTDATA("data_in/NGSDImportHPO_terms.obo") + " -anno " + TESTDATA("data_in/NGSDImportHPO_anno.txt") + " -omim " + TESTDATA("data_in/NGSDImportHPO_omim.txt") + " -clinvar " + TESTDATA("data_in/NGSDImportHPO_clinvar.txt") + " -hgmd " + TESTDATA("data_in/NGSDImportHPO_hgmd.dump") + " -hpophen " + TESTDATA("data_in/NGSDImportHPO_phenotype.hpoa") + " -gencc " + TESTDATA("data_in/NGSDImportHPO_gencc.csv") + " -g2p " + TESTDATA("data_in/NGSDImportHPO_decipher1.csv") + " -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM hpo_term").toInt();
		I_EQUAL(count, 15)
		count = db.getValue("SELECT count(*) FROM hpo_parent").toInt();
		I_EQUAL(count, 11)
		count = db.getValue("SELECT count(*) FROM hpo_genes").toInt();
		I_EQUAL(count, 180);
	}
};

