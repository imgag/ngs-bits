#include "TestFramework.h"
#include "NGSD.h"
#include "Settings.h"
#include <QSqlRecord>

TEST_CLASS(NGSDAddVariantsSomatic_Test)
{
Q_OBJECT
private slots:
	void test_small_variants_tumor_normal()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

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

		//test callset was imported correctly
		//TODO Alexander: add somatic callset for small variants to NGSD

		//force variant import
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -n_ps DX184263_01 -force -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in1.GSvar"));
		//should fail because variants already exist an force is unset
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -n_ps DX184263_01 -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in1.GSvar"));
	}

	void test_small_variants_tumor_only()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

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
		//TODO Alexander: add somatic callset for small variants to NGSD

		//force variant import
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -force -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in3.GSvar"));
		//should fail because variants already exist an force is unset
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in3.GSvar"));
	}

	void test_cnvs()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

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

		//Cnvs already imported
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -n_ps DX184263_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in2.tsv"));
		//Cnvs already imported force
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -force -t_ps DX184894_01 -n_ps DX184263_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in2.tsv"));
	}

	void test_svs()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -n_ps DX184263_01 -sv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in4.bedpe"));

		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_deletion").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_duplication").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_insertion").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_inversion").toInt(), 17);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_deletion").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_sv_deletion").toInt(), 0);

		//test callset was imported correctly
		QString ps_t = db.processedSampleId("DX184894_01");
		S_EQUAL(db.getValue("SELECT caller FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "Manta");
		S_EQUAL(db.getValue("SELECT caller_version FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toString(), "1.6.0");
		S_EQUAL(db.getValue("SELECT call_date FROM somatic_sv_callset WHERE ps_tumor_id='"+ps_t+"'").toDate().toString(Qt::ISODate), "2025-05-19");

		//Cnvs already imported
		EXECUTE_FAIL("NGSDAddVariantsSomatic", "-test -debug -no_time -t_ps DX184894_01 -n_ps DX184263_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in2.tsv"));
		//Cnvs already imported force
		EXECUTE("NGSDAddVariantsSomatic", "-test -debug -no_time -force -t_ps DX184894_01 -n_ps DX184263_01 -cnv " + TESTDATA("data_in/NGSDAddVariantsSomatic_in2.tsv"));
	}
};
