#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportStudyGHGA_Test)
{
Q_OBJECT
private slots:

	void test_default()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.tsv") + " -test -include_bam -out out/NGSDExportStudyGHGA_out1.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out1.json", TESTDATA("data_out/NGSDExportStudyGHGA_out1.json"));
	}

	void test_vcf_patid()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in2.tsv") + " -test -include_bam -include_vcf -out out/NGSDExportStudyGHGA_out2.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out2.json", TESTDATA("data_out/NGSDExportStudyGHGA_out2.json"));
	}

	void test_use_sample_folder()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in3.tsv") + " -test "
				+ "-use_sample_folder -include_bam -include_vcf -out out/NGSDExportStudyGHGA_out3.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out3.json", TESTDATA("data_out/NGSDExportStudyGHGA_out3.json"));
	}

	void test_group_analyses()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportStudyGHGA_init.sql"));

		//test
		EXECUTE("NGSDExportStudyGHGA", "-data " + TESTDATA("data_in/NGSDExportStudyGHGA_in1.json") + " -samples " + TESTDATA("data_in/NGSDExportStudyGHGA_in3.tsv") + " -test "
				+ "-use_sample_folder -group_analyses -out out/NGSDExportStudyGHGA_out4.json");
		COMPARE_FILES("out/NGSDExportStudyGHGA_out4.json", TESTDATA("data_out/NGSDExportStudyGHGA_out4.json"));
	}

};
