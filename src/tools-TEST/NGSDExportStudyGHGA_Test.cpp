#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportStudyGHGA_Test)
{
private:

	TEST_METHOD(test_default)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.tsv") + " -test -include_bam -out out/NGSDExportStudyGHGA_out1.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out1.json", TESTDATA("data_out/NGSDExportStudyGHGA_out1.json"));
	}

	TEST_METHOD(test_vcf_patid)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in2.tsv") + " -test -include_bam -include_vcf -out out/NGSDExportStudyGHGA_out2.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out2.json", TESTDATA("data_out/NGSDExportStudyGHGA_out2.json"));
	}

	TEST_METHOD(test_use_sample_folder)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in3.tsv") + " -test "
				+ "-use_sample_folder -include_bam -include_vcf -out out/NGSDExportStudyGHGA_out3.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out3.json", TESTDATA("data_out/NGSDExportStudyGHGA_out3.json"));
	}

	TEST_METHOD(test_group_analyses)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in3.tsv") + " -test "
				+ "-use_sample_folder -group_analyses -include_vcf -out out/NGSDExportStudyGHGA_out4.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out4.json", TESTDATA("data_out/NGSDExportStudyGHGA_out4.json"));
	}

	TEST_METHOD(test_fastq_only)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in3.tsv") + " -test "
				+ "-use_sample_folder -out out/NGSDExportStudyGHGA_out5.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out5.json", TESTDATA("data_out/NGSDExportStudyGHGA_out5.json"));
	}

};
