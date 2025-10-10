#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDSameSample_Test)
{
private:
	
	TEST_METHOD(same_sample)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -mode SAME_SAMPLE -ps NA12880_01 -out out/NGSDSameSample_out1.tsv");
		COMPARE_FILES("out/NGSDSameSample_out1.tsv", TESTDATA("data_out/NGSDSameSample_out1.tsv"));
	}

	TEST_METHOD(same_patient)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -mode SAME_PATIENT -ps NA12880_01 -out out/NGSDSameSample_out2.tsv");
		COMPARE_FILES("out/NGSDSameSample_out2.tsv", TESTDATA("data_out/NGSDSameSample_out2.tsv"));
	}

	TEST_METHOD(dna_only_with_bad)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -sample_type DNA -ps NA12880_01 -out out/NGSDSameSample_out3.tsv -include_bad");
		COMPARE_FILES("out/NGSDSameSample_out3.tsv", TESTDATA("data_out/NGSDSameSample_out3.tsv"));
	}

	TEST_METHOD(only_wgs)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -system_type WGS,lrGS -ps NA12880_01 -out out/NGSDSameSample_out4.tsv");
		COMPARE_FILES("out/NGSDSameSample_out4.tsv", TESTDATA("data_out/NGSDSameSample_out4.tsv"));
	}

	TEST_METHOD(only_nanopore)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -system SQK-114 -ps NA12880_01 -out out/NGSDSameSample_out5.tsv");
		COMPARE_FILES("out/NGSDSameSample_out5.tsv", TESTDATA("data_out/NGSDSameSample_out5.tsv"));
	}

	TEST_METHOD(include_merged)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -include_merged -system_type WGS,lrGS -ps NA12880_01 -out out/NGSDSameSample_out6.tsv");
		COMPARE_FILES("out/NGSDSameSample_out6.tsv", TESTDATA("data_out/NGSDSameSample_out6.tsv"));
	}


};
