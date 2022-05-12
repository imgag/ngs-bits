#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDAddVariantsGermline_Test)
{
Q_OBJECT
private slots:

	//CnvHunter cnvs
	void test_panel()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		//1. import
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line21.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line21.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line21.log", TESTDATA("data_out/NGSDAddVariantsGermline_out1.log"));

		//2. import - to check that reimporting works
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv") + " -var_force -cnv_force");
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line27.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line27.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line27.log", TESTDATA("data_out/NGSDAddVariantsGermline_out2.log"));

		//3. import - test that updating of small variants works
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.1.GSvar") + " -var_update");
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line33.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line33.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line33.log", TESTDATA("data_out/NGSDAddVariantsGermline_out3.log"));
	}

	//ClinCNV cnvs
	void test_wes()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		//import
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_38 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in2.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in2.tsv"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line50.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line50.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line50.log", TESTDATA("data_out/NGSDAddVariantsGermline_out4.log"));

		//check if PubMed ids are imported
		Variant var = Variant(Chromosome("chrX"), 155255024, 155255024, "C", "T");
		QString var_id = db.variantId(var);
		QStringList pubmed_ids = db.pubmedIds(var_id);
		pubmed_ids.sort();
		S_EQUAL(pubmed_ids.at(0), "12345678");
		S_EQUAL(pubmed_ids.at(1), "87654321")
	}

	void sv_default_import()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 35);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 8);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 36);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 6);
		count = db.getValue("SELECT count(*) FROM sv_callset").toInt();
		I_EQUAL(count, 1);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line73.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line73.log", TESTDATA("data_out/NGSDAddVariantsGermline_out5.log"));
	}


	void sv_failed_reimport()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//re-import SVs for same sample without "-force" (no change)
		EXECUTE_FAIL("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in_empty.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 35);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 8);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 36);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 6);
		count = db.getValue("SELECT count(*) FROM sv_callset").toInt();
		I_EQUAL(count, 1);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line104.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line104.log", TESTDATA("data_out/NGSDAddVariantsGermline_out6.log"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line107.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line107.log", TESTDATA("data_out/NGSDAddVariantsGermline_out7.log"));
	}


	void sv_forced_reimport()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//re-import SVs for same sample with "-force" (overwrite)
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv_force -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in_empty.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_callset").toInt();
		I_EQUAL(count, 1);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line140.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line140.log", TESTDATA("data_out/NGSDAddVariantsGermline_out8.log"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line143.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line143.log", TESTDATA("data_out/NGSDAddVariantsGermline_out9.log"));
	}

	void import_with_existing_report_config()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_report_config.sql"));

		//try to import variants
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -cnv_force -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv"));
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv_force -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM variant").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM cnv").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 0);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line177.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line177.log", TESTDATA("data_out/NGSDAddVariantsGermline_out10.log"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line178.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line178.log", TESTDATA("data_out/NGSDAddVariantsGermline_out11.log"));
	}
};


