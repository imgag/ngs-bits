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
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		//1. import
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv") );
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line22.log", TESTDATA("data_out/NGSDAddVariantsGermline_out1.log"));

		//2. import - to check that updating works
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv") + " -var_force -cnv_force");
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line26.log", TESTDATA("data_out/NGSDAddVariantsGermline_out2.log"));
	}

	//ClinCNV cnvs
	void test_wes()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		//import
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_38 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in2.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in2.tsv"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line42.log", TESTDATA("data_out/NGSDAddVariantsGermline_out3.log"));
	}

	void sv_default_import()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

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
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line56.log", TESTDATA("data_out/NGSDAddVariantsGermline_out56.log"));
	}


	void sv_failed_reimport()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

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
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line87.log", TESTDATA("data_out/NGSDAddVariantsGermline_out87.log"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line90.log", TESTDATA("data_out/NGSDAddVariantsGermline_out90.log"));
	}


	void sv_forced_reimport()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

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
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line122.log", TESTDATA("data_out/NGSDAddVariantsGermline_out122.log"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line125.log", TESTDATA("data_out/NGSDAddVariantsGermline_out125.log"));
	}
};


