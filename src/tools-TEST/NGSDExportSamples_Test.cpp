#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportSamples_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -out out/NGSDExportSamples_out1.tsv");
		COMPARE_FILES("out/NGSDExportSamples_out1.tsv", TESTDATA("data_out/NGSDExportSamples_out1.tsv"));
	}

	TEST_METHOD(with_all_optional_output)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -add_disease_details -add_outcome -add_qc -add_report_config -add_comments -add_normal_sample -add_dates -add_call_details -add_lab_columns -add_study_column -out out/NGSDExportSamples_out2.tsv");
		COMPARE_FILES("out/NGSDExportSamples_out2.tsv", TESTDATA("data_out/NGSDExportSamples_out2.tsv"));
	}

	TEST_METHOD(with_all_search_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -sample NA12878 -species human -disease_group Neoplasms -disease_status Affected -tissue blood -no_bad_samples -no_tumor -no_ffpe -project Second_project -project_type diagnostic -system ssHAEv5 -system_type WGS -run run2 -no_bad_runs -run_device Morpheus -out out/NGSDExportSamples_out3.tsv -sender Klaus-Erhard -study SomeStudy -no_archived_projects -phenotypes HP:0000003;HP:0002862 -ancestry EUR");
		COMPARE_FILES("out/NGSDExportSamples_out3.tsv", TESTDATA("data_out/NGSDExportSamples_out3.tsv"));
	}
	TEST_METHOD(only_tumor)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -out out/NGSDExportSamples_out4.tsv -no_normal");
		COMPARE_FILES("out/NGSDExportSamples_out4.tsv", TESTDATA("data_out/NGSDExportSamples_out4.tsv"));
	}


	TEST_METHOD(ps_override)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test on CLI
		EXECUTE("NGSDExportSamples", "-test -out out/NGSDExportSamples_out5.tsv -ps_override NA12880_01;NA12878_01");
		COMPARE_FILES("out/NGSDExportSamples_out5.tsv", TESTDATA("data_out/NGSDExportSamples_out5.tsv"));

		//test with file
		EXECUTE("NGSDExportSamples", "-test -out out/NGSDExportSamples_out5_v2.tsv -ps_override "+TESTDATA("data_in/NGSDExportSamples_ps_override.sql"));
		COMPARE_FILES("out/NGSDExportSamples_out5_v2.tsv", TESTDATA("data_out/NGSDExportSamples_out5.tsv"));
	}
};
