#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportSamples_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -out out/NGSDExportSamples_out1.tsv");
		COMPARE_FILES("out/NGSDExportSamples_out1.tsv", TESTDATA("data_out/NGSDExportSamples_out1.tsv"));
	}

	void with_all_optional_output()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -add_disease_details -add_outcome -add_qc -add_report_config -add_comments -add_normal_sample -add_dates -add_call_details -add_lab_columns -add_study_column -out out/NGSDExportSamples_out2.tsv");
		COMPARE_FILES("out/NGSDExportSamples_out2.tsv", TESTDATA("data_out/NGSDExportSamples_out2.tsv"));
	}

	void with_all_search_parameters()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -sample NA12878 -species human -disease_group Neoplasms -disease_status Affected -tissue blood -no_bad_samples -no_tumor -no_ffpe -project Second_project -project_type diagnostic -system ssHAEv5 -system_type WGS -run run2 -no_bad_runs -run_device Morpheus -out out/NGSDExportSamples_out3.tsv -sender Klaus-Erhard -study SomeStudy -no_archived_projects -phenotypes HP:0000003;HP:0002862 -ancestry EUR");
		COMPARE_FILES("out/NGSDExportSamples_out3.tsv", TESTDATA("data_out/NGSDExportSamples_out3.tsv"));
	}

	void only_tumor()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSamples_init.sql"));

		//test
		EXECUTE("NGSDExportSamples", "-test -out out/NGSDExportSamples_out4.tsv -no_normal");
		COMPARE_FILES("out/NGSDExportSamples_out4.tsv", TESTDATA("data_out/NGSDExportSamples_out4.tsv"));
	}
};
