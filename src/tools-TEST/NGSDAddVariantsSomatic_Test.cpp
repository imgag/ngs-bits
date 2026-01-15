#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDAddVariantsSomatic_Test)
{
private:
	TEST_METHOD(test_small_variants_tumor_normal)
	{
		SKIP_IF_NO_TEST_NGSD();

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -n_ps DX184263_01 -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in1.GSvar"));

		S_EQUAL(db.variant("1").toString(), "chr2:178096717-178096717 T>C");
		S_EQUAL(db.variant("2").toString(), "chr3:138456487-138456488 AT>-");
		S_EQUAL(db.variant("3").toString(), "chr16:56870524-56870524 A>C");

		//Check variant entries in detected_somatic_variants
		DBTable table = db.createTable("test", "SELECT * FROM detected_somatic_variant");
		I_EQUAL(table.rowCount(), 3);
		S_EQUAL(table.row(0).asString(';'), "1;8;7;1;0.1057;389;229");
		S_EQUAL(table.row(1).asString(';'), "2;8;7;2;0.1304;26;22");
		S_EQUAL(table.row(2).asString(';'), "3;8;7;3;0.1254;639;330");

		QString t_ps_id = db.processedSampleId("DX184894_01");
		QString n_ps_id = db.processedSampleId("DX184263_01");
		//test callset was imported correctly
		S_EQUAL(db.getValue("SELECT caller FROM somatic_snv_callset WHERE processed_sample_id_tumor='"+t_ps_id+"' AND processed_sample_id_normal='"+n_ps_id+"'").toString(), "strelka2");
		S_EQUAL(db.getValue("SELECT caller_version FROM somatic_snv_callset WHERE processed_sample_id_tumor='"+t_ps_id+"' AND processed_sample_id_normal='"+n_ps_id+"'").toString(), "2.9.10");
		S_EQUAL(db.getValue("SELECT call_date FROM somatic_snv_callset WHERE processed_sample_id_tumor='"+t_ps_id+"' AND processed_sample_id_normal='"+n_ps_id+"'").toDate().toString(Qt::ISODate), "2020-10-10");

		//force variant import
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -n_ps DX184263_01 -force -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in1.GSvar"));
		//should fail because variants already exist and force is unset
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -n_ps DX184263_01 -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in1.GSvar"));
	}

	TEST_METHOD(test_small_variants_tumor_only)
	{
		SKIP_IF_NO_TEST_NGSD();

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in3.GSvar"));

		S_EQUAL(db.variant("1").toString(), "chr2:178096717-178096717 T>C");
		S_EQUAL(db.variant("2").toString(), "chr16:56870524-56870524 A>C");

		//Check variant entries in detected_somatic_variants
		DBTable table = db.createTable("test", "SELECT * FROM detected_somatic_variant");
		I_EQUAL(table.rowCount(), 2);
		S_EQUAL(table.row(0).asString(';'), "1;8;;1;0.1057;389;229");
		S_EQUAL(table.row(1).asString(';'), "2;8;;2;0.1254;639;330");

		//test callset was imported correctly
		QString t_ps_id = db.processedSampleId("DX184894_01");
		//test callset was imported correctly
		S_EQUAL(db.getValue("SELECT caller FROM somatic_snv_callset WHERE processed_sample_id_tumor='"+t_ps_id+"' AND processed_sample_id_normal IS NULL").toString(), "VarScan2");
		S_EQUAL(db.getValue("SELECT caller_version FROM somatic_snv_callset WHERE processed_sample_id_tumor='"+t_ps_id+"' AND processed_sample_id_normal IS NULL").toString(), "v2.4.6");
		S_EQUAL(db.getValue("SELECT call_date FROM somatic_snv_callset WHERE processed_sample_id_tumor='"+t_ps_id+"' AND processed_sample_id_normal IS NULL").toDate().toString(Qt::ISODate), "2020-10-10");

		//force variant import
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -force -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in3.GSvar"));
		//should fail because variants already exist and force is unset
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in3.GSvar"));
	}

	TEST_METHOD(test_cnvs)
	{
		SKIP_IF_NO_TEST_NGSD();

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -n_ps DX184263_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in2.tsv"));

		DBTable table = db.createTable("test", "SELECT * FROM somatic_cnv");

		I_EQUAL(table.rowCount(), 3); //only 4 imported CNVs, because one is skipped (min_ll)";
		S_EQUAL(table.row(0).asString(';'), "1;1;chr1;32097627;32860246;2.775;3;0.775;{\"BAF_qval_fdr\":\"NA\",\"Highmed_tumor_BAF\":\"NA\",\"Lowmed_tumor_BAF\":\"NA\",\"Offtarget_RD_CI_lower\":\"2.43\",\"Offtarget_RD_CI_upper\":\"2.99\",\"Ontarget_RD_CI_lower\":\"NA\",\"Ontarget_RD_CI_upper\":\"NA\",\"Overall_qvalue\":\"0\",\"loglikelihood\":\"191\",\"major_CN_allele\":\"2\",\"minor_CN_allele\":\"1\",\"regions\":\"8\"}");
		S_EQUAL(table.row(1).asString(';'), "2;1;chr11;26582421;27694430;2;2;0.775;{\"BAF_qval_fdr\":\"NA\",\"Highmed_tumor_BAF\":\"NA\",\"Lowmed_tumor_BAF\":\"NA\",\"Offtarget_RD_CI_lower\":\"2.51\",\"Offtarget_RD_CI_upper\":\"3.19\",\"Ontarget_RD_CI_lower\":\"NA\",\"Ontarget_RD_CI_upper\":\"NA\",\"Overall_qvalue\":\"0\",\"loglikelihood\":\"248\",\"major_CN_allele\":\"2\",\"minor_CN_allele\":\"1\",\"regions\":\"11\"}");
		S_EQUAL(table.row(2).asString(';'), "3;1;chr17;45660082;46365906;1.25;1;0.75;{\"BAF_qval_fdr\":\"NA\",\"Highmed_tumor_BAF\":\"NA\",\"Lowmed_tumor_BAF\":\"NA\",\"Offtarget_RD_CI_lower\":\"1.17\",\"Offtarget_RD_CI_upper\":\"1.48\",\"Ontarget_RD_CI_lower\":\"NA\",\"Ontarget_RD_CI_upper\":\"NA\",\"Overall_qvalue\":\"0\",\"loglikelihood\":\"275\",\"major_CN_allele\":\"1\",\"minor_CN_allele\":\"0\",\"regions\":\"7\"}");

		//test callset was imported correctly
		QString ps_t = db.processedSampleId("DX184894_01");
		S_EQUAL(db.getValue("SELECT caller FROM somatic_cnv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "ClinCNV");
		S_EQUAL(db.getValue("SELECT caller_version FROM somatic_cnv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "v1.16.1");
		S_EQUAL(db.getValue("SELECT call_date FROM somatic_cnv_callset WHERE ps_tumor_id='"+ps_t+"'").toDate().toString(Qt::ISODate), "2019-10-06");

		//CNVs already imported
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -n_ps DX184263_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in2.tsv"));
		//CNVs already imported - force
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -force -t_ps DX184894_01 -n_ps DX184263_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in2.tsv"));
	}

	TEST_METHOD(test_cnvs_tumor_only)
	{
		SKIP_IF_NO_TEST_NGSD();

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in5.tsv"));

		DBTable table = db.createTable("test", "SELECT * FROM somatic_cnv");

		I_EQUAL(table.rowCount(), 1);
		S_EQUAL(table.row(0).asString(';'), "1;1;chr1;3901206;5765702;1.4;0;0.3;{\"Major allele\":\"0\",\"Minor allele\":\"0\",\"loglikelihood\":\"226\",\"regions\":\"28\"}");

		//test callset was imported correctly
		QString ps_t = db.processedSampleId("DX184894_01");
		S_EQUAL(db.getValue("SELECT caller FROM somatic_cnv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "ClinCNV");
		S_EQUAL(db.getValue("SELECT caller_version FROM somatic_cnv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "v1.18.3");
		S_EQUAL(db.getValue("SELECT call_date FROM somatic_cnv_callset WHERE ps_tumor_id='"+ps_t+"'").toDate().toString(Qt::ISODate), "2025-11-27");

		//CNVs already imported
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in5.tsv"));
		//CNVs already imported - force
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -force -t_ps DX184894_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in5.tsv"));
	}

	TEST_METHOD(test_svs)
	{
		SKIP_IF_NO_TEST_NGSD();

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -n_ps DX184263_01 -sv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in4.bedpe"));

		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_deletion").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_duplication").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_insertion").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_inversion").toInt(), 17);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_translocation").toInt(), 0);

		//test callset was imported correctly
		QString ps_t = db.processedSampleId("DX184894_01");
		S_EQUAL(db.getValue("SELECT caller FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "Manta");
		S_EQUAL(db.getValue("SELECT caller_version FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "1.6.0");
		S_EQUAL(db.getValue("SELECT call_date FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toDate().toString(Qt::ISODate), "2025-05-19");

		//SVs already imported
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -n_ps DX184263_01 -sv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in4.bedpe"));
		//SVs already imported - force
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -force -t_ps DX184894_01 -n_ps DX184263_01 -sv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in4.bedpe"));
	}

	TEST_METHOD(test_svs_tumor_only)
	{
		SKIP_IF_NO_TEST_NGSD();

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -sv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in6.bedpe"));

		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_deletion").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_duplication").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_insertion").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_inversion").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_translocation").toInt(), 0);

		//test callset was imported correctly
		QString ps_t = db.processedSampleId("DX184894_01");
		S_EQUAL(db.getValue("SELECT caller FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "Manta");
		S_EQUAL(db.getValue("SELECT caller_version FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "1.6.1");
		S_EQUAL(db.getValue("SELECT call_date FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toDate().toString(Qt::ISODate), "2025-11-27");

		//SVs already imported
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -sv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in6.bedpe"));
		//SVs already imported - force
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -force -t_ps DX184894_01 -sv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in6.bedpe"));
	}
};
