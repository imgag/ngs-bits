#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportExpressionData_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		//import sample
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportExpressionData_init1.sql"));

		//test
		EXECUTE("NGSDImportExpressionData", "-test -expression " + TESTDATA("data_in/NGSDImportExpressionData_in1_counts.tsv") + " -ps RX123456_03 -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 7997);
	}

	TEST_METHOD(forced_import)
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		//import sample
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportExpressionData_init1.sql"));

		//initial import
		EXECUTE("NGSDImportExpressionData", "-test -expression " + TESTDATA("data_in/NGSDImportExpressionData_in1_counts.tsv") + " -ps RX123456_03 -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 7997);

		//try without 'force' parameter
		EXECUTE_FAIL("NGSDImportExpressionData", "-test -expression " + TESTDATA("data_in/NGSDImportExpressionData_in2_counts.tsv") + " -ps RX123456_03 -debug");
		//check if import failed
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 7997);

		//try with 'force' parameter
		EXECUTE("NGSDImportExpressionData", "-test -force -expression " + TESTDATA("data_in/NGSDImportExpressionData_in2_counts.tsv") + " -ps RX123456_03 -debug");
		//check if import worked
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 102);

	}

	TEST_METHOD(exon_import)
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		//import sample
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportExpressionData_init2.sql"));

		//test
		EXECUTE("NGSDImportExpressionData", "-test -expression " + TESTDATA("data_in/NGSDImportExpressionData_in1_exon.tsv") + " -ps RX123456_03 -mode exons -debug");

		//check
		int count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 71);

		//try without 'force' parameter
		EXECUTE_FAIL("NGSDImportExpressionData", "-test -expression " + TESTDATA("data_in/NGSDImportExpressionData_in2_exon.tsv") + " -ps RX123456_03 -mode exons -debug");
		//check if import failed
		count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 71);

		//try with 'force' parameter
		EXECUTE("NGSDImportExpressionData", "-test -force -expression " + TESTDATA("data_in/NGSDImportExpressionData_in2_exon.tsv") + " -ps RX123456_03 -mode exons -debug");
		//check if import worked
		count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 43);
	}


};

