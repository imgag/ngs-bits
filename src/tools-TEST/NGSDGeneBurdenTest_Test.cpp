#include "TestFrameworkNGS.h"
#include "NGSD.h"


TEST_CLASS(NGSDGeneBurdenTest_Test)
{
private:

    //test with default parameters
    TEST_METHOD(default_params)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt") + " -out out/NGSDGeneBurdenTest_out1.tsv -debug -test");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out1.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out1.tsv"));
    }

	TEST_METHOD(singlethread)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt") + " -out out/NGSDGeneBurdenTest_out1.tsv -debug -test -threads 1");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out1.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out1.tsv"));
	}

	//test with default parameters
	TEST_METHOD(low_impact_modifier)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt") + " -debug -test -impacts HIGH,MODERATE,LOW"
										  + " -out out/NGSDGeneBurdenTest_out2.tsv ");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out2.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out2.tsv"));
	}

	//test with default parameters
	TEST_METHOD(predict_pathogenic)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt") + " -debug -test -predict_pathogenic"
										  + " -out out/NGSDGeneBurdenTest_out3.tsv ");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out3.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out3.tsv"));
	}

	TEST_METHOD(recessive)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt") + " -out out/NGSDGeneBurdenTest_out4.tsv -debug -test -inheritance recessive");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out4.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out4.tsv"));
	}

	TEST_METHOD(ccr_region)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt")
										  + " -out out/NGSDGeneBurdenTest_out5.tsv -ccr_only -impacts HIGH,MODERATE,LOW,MODIFIER -debug -test");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out5.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out5.tsv"));
	}

	TEST_METHOD(ngsd_count)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt") + " -out out/NGSDGeneBurdenTest_out6.tsv -debug -test -max_ngsd_count 15");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out6.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out6.tsv"));
	}

	TEST_METHOD(cnv)
	{
		SKIP_IF_NO_TEST_NGSD();
		SKIP_IF_NO_HG38_GENOME();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDGeneBurdenTest_in.sql"));
		//import transcripts
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDGeneBurdenTest_in.gff3"));


		EXECUTE("NGSDGeneBurdenTest", " -cases " + TESTDATA("data_in/NGSDGeneBurdenTest_in_cases.txt") + " -controls " + TESTDATA("data_in/NGSDGeneBurdenTest_in_controls.txt")
										  + " -genes " + TESTDATA("data_in/NGSDGeneBurdenTest_in_genes.txt") + " -out out/NGSDGeneBurdenTest_out7.tsv -debug -test -include_cnvs");

		COMPARE_FILES("out/NGSDGeneBurdenTest_out7.tsv", TESTDATA("data_out/NGSDGeneBurdenTest_out7.tsv"));
	}

};
