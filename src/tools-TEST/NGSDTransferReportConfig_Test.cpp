#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDTransferReportConfig_Test)
{
private:
	
	TEST_METHOD(fails_missing_and_mismatch_variants)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDTransferReportConfig_in1.sql"));
		//set sample folder
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_05/") + "' WHERE id=1");
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_06/") + "' WHERE id=2");


		EXECUTE_FAIL("NGSDTransferReportConfig", "-test -source_ps NA12878_05 -target_ps NA12878_06");

		//check stdout
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line21.log", QRegularExpression("^NGSDTransferReportConfig "));
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line21.log", QRegularExpression("^Location"));
		COMPARE_FILES("out/NGSDTransferReportConfig_Test_line21.log", TESTDATA("data_out/NGSDTransferReportConfig_Test_fails_missing_and_mismatch_variants.log"));
	}

	TEST_METHOD(fails_mismatch_variants)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDTransferReportConfig_in1.sql"));
		//set sample folder
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_05/") + "' WHERE id=1");
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_06/") + "' WHERE id=2");

		//delete missing variants
		db.getQuery().exec("DELETE FROM `report_configuration_variant` WHERE id=4");
		db.getQuery().exec("DELETE FROM `report_configuration_variant` WHERE id=5");
		db.getQuery().exec("DELETE FROM `report_configuration_cnv` WHERE id=4");
		db.getQuery().exec("DELETE FROM `report_configuration_sv` WHERE id=5");
		db.getQuery().exec("DELETE FROM `report_configuration_re` WHERE id=5");

		EXECUTE_FAIL("NGSDTransferReportConfig", "-test -source_ps NA12878_05 -target_ps NA12878_06");

		//check stdout
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line48.log", QRegularExpression("^NGSDTransferReportConfig "));
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line48.log", QRegularExpression("^Location"));
		COMPARE_FILES("out/NGSDTransferReportConfig_Test_line48.log", TESTDATA("data_out/NGSDTransferReportConfig_Test_fails_mismatch_variants.log"));
	}

	TEST_METHOD(fails_no_report_config)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDTransferReportConfig_in1.sql"));
		//set sample folder
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_05/") + "' WHERE id=1");
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_06/") + "' WHERE id=2");

		//delete ReportConfig
		db.deleteReportConfig(1);

		EXECUTE_FAIL("NGSDTransferReportConfig", "-test -source_ps NA12878_05 -target_ps NA12878_06");

		//check stdout
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line71.log", QRegularExpression("^NGSDTransferReportConfig "));
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line71.log", QRegularExpression("^Location"));
		COMPARE_FILES("out/NGSDTransferReportConfig_Test_line71.log", TESTDATA("data_out/NGSDTransferReportConfig_Test_fails_no_report_config.log"));
	}

	TEST_METHOD(fails_target_has_report_config)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDTransferReportConfig_in1.sql"));
		//set sample folder
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_05/") + "' WHERE id=1");
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_06/") + "' WHERE id=2");

		//create ReportConfig for NA12878_06
		db.getQuery().exec(QString("INSERT INTO `report_configuration`(`id`, `processed_sample_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`) VALUES ")
						   + "(2,2,100,'2000-01-01 11:11:11',101,'2020-01-01 22:22:22');");

		EXECUTE_FAIL("NGSDTransferReportConfig", "-test -source_ps NA12878_05 -target_ps NA12878_06");

		//check stdout
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line95.log", QRegularExpression("^NGSDTransferReportConfig "));
		REMOVE_LINES("out/NGSDTransferReportConfig_Test_line95.log", QRegularExpression("^Location"));
		COMPARE_FILES("out/NGSDTransferReportConfig_Test_line95.log", TESTDATA("data_out/NGSDTransferReportConfig_Test_fails_target_has_report_config.log"));
	}

	TEST_METHOD(only_allowed_mismatches)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDTransferReportConfig_in1.sql"));
		//set sample folder
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_05/") + "' WHERE id=1");
		db.getQuery().exec("UPDATE processed_sample SET folder_override='" + TESTDATA("data_in/NGSDTransferReportConfig/Sample_NA12878_06/") + "' WHERE id=2");

		//delete missing variants
		db.getQuery().exec("DELETE FROM `report_configuration_variant` WHERE id=4");
		db.getQuery().exec("DELETE FROM `report_configuration_variant` WHERE id=5");
		db.getQuery().exec("DELETE FROM `report_configuration_cnv` WHERE id=4");
		db.getQuery().exec("DELETE FROM `report_configuration_sv` WHERE id=5");
		db.getQuery().exec("DELETE FROM `report_configuration_re` WHERE id=5");

		//delete variants with too large mismatch
		db.getQuery().exec("DELETE FROM `report_configuration_cnv` WHERE id=3");
		db.getQuery().exec("DELETE FROM `report_configuration_sv` WHERE id=4");
		db.getQuery().exec("DELETE FROM `report_configuration_re` WHERE id=4");

		EXECUTE("NGSDTransferReportConfig", "-test -source_ps NA12878_05 -target_ps NA12878_06");

		//check stdout
		COMPARE_FILES("out/NGSDTransferReportConfig_Test_line127.log", TESTDATA("data_out/NGSDTransferReportConfig_Test_only_allowed_mismatches.log"));

		//compare both configs

		//compare meta-data
		SqlQuery rc1_query = db.getQuery();
		rc1_query.exec("SELECT * FROM `report_configuration` WHERE id=1");
		rc1_query.next();
		SqlQuery rc2_query = db.getQuery();
		rc2_query.exec("SELECT * FROM `report_configuration` WHERE id=2");
		rc2_query.next();
		IS_TRUE(rc1_query.value("created_by") == rc2_query.value("created_by"));
		IS_TRUE(rc1_query.value("created_date") == rc2_query.value("created_date"));
		IS_TRUE(rc1_query.value("last_edit_by") == rc2_query.value("last_edit_by"));
		IS_TRUE(rc1_query.value("last_edit_date") == rc2_query.value("last_edit_date"));
		IS_TRUE(rc1_query.value("finalized_by") == rc2_query.value("finalized_by"));
		IS_TRUE(rc1_query.value("finalized_date") == rc2_query.value("finalized_date"));

		//compare other causal variant:
		SqlQuery ocv1_query = db.getQuery();
		ocv1_query.exec("SELECT * FROM `report_configuration_other_causal_variant` WHERE report_configuration_id=1");
		ocv1_query.next();
		SqlQuery ocv2_query = db.getQuery();
		ocv2_query.exec("SELECT * FROM `report_configuration_other_causal_variant` WHERE report_configuration_id=2");
		ocv2_query.next();
		IS_TRUE(ocv1_query.value("coordinates") == ocv2_query.value("coordinates"));
		IS_TRUE(ocv1_query.value("gene") == ocv2_query.value("gene"));
		IS_TRUE(ocv1_query.value("type") == ocv2_query.value("type"));
		IS_TRUE(ocv1_query.value("inheritance") == ocv2_query.value("inheritance"));
		IS_TRUE(ocv1_query.value("comment") == ocv2_query.value("comment"));
		IS_TRUE(ocv1_query.value("comment_reviewer1") == ocv2_query.value("comment_reviewer1"));
		IS_TRUE(ocv1_query.value("comment_reviewer2") == ocv2_query.value("comment_reviewer2"));

		//check counts of transfered report config
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_variant` WHERE report_configuration_id=2").toInt(), 3);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_cnv` WHERE report_configuration_id=2").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_sv` WHERE report_configuration_id=2").toInt(), 3);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_re` WHERE report_configuration_id=2").toInt(), 3);

		//check duplicate variants:
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_variant` WHERE variant_id=3").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_variant` WHERE variant_id=6").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_variant` WHERE variant_id=9").toInt(), 2);

		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_cnv` WHERE comments='exact match'").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_cnv` WHERE comments='partial match (<90%)'").toInt(), 2);

		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_sv` WHERE sv_deletion_id IS NOT NULL").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_sv` WHERE sv_duplication_id IS NOT NULL").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_sv` WHERE sv_translocation_id IS NOT NULL").toInt(), 2);

		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_re` WHERE comments='exact match'").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_re` WHERE comments='smaller allele changes'").toInt(), 2);
		I_EQUAL(db.getValue("SELECT COUNT(id) FROM `report_configuration_re` WHERE comments='larger allele changes in allowed range'").toInt(), 2);

	}

};
