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
};


