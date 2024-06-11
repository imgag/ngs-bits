#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDAddVariantsGermline_Test)
{
Q_OBJECT
private slots:

	void test_panel()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		//1. import
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line21.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line21.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line21.log", TESTDATA("data_out/NGSDAddVariantsGermline_out1.log"));

		//check callset (freebayes)
		S_EQUAL(db.getValue("SELECT caller FROM small_variants_callset WHERE processed_sample_id='3999'").toString(), "freebayes");
		S_EQUAL(db.getValue("SELECT caller_version FROM small_variants_callset WHERE processed_sample_id='3999'").toString(), "v1.3.3");
		S_EQUAL(db.getValue("SELECT call_date FROM small_variants_callset WHERE processed_sample_id='3999'").toDate().toString(Qt::ISODate), "2022-04-25");

		//2. import - to check that reimporting works
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv") + " -var_force -cnv_force");
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line32.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line32.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line32.log", TESTDATA("data_out/NGSDAddVariantsGermline_out2.log"));

		//3. import - test that updating of small variants works
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_18 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.1.GSvar") + " -var_update");
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line38.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line38.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line38.log", TESTDATA("data_out/NGSDAddVariantsGermline_out3.log"));

		//check callset (Clair3)
		S_EQUAL(db.getValue("SELECT caller FROM small_variants_callset WHERE processed_sample_id='3999'").toString(), "Clair3");
		S_EQUAL(db.getValue("SELECT caller_version FROM small_variants_callset WHERE processed_sample_id='3999'").toString(), "1.0.0");
		S_EQUAL(db.getValue("SELECT call_date FROM small_variants_callset WHERE processed_sample_id='3999'").toDate().toString(Qt::ISODate), "2023-11-03");
	}

	void test_wes()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		//import
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_38 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in2.GSvar") + " -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in2.tsv"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line60.log", QRegExp("^WARNING: transactions"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line60.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line60.log", TESTDATA("data_out/NGSDAddVariantsGermline_out4.log"));

		//check callset (DRAGEN)
		S_EQUAL(db.getValue("SELECT caller FROM small_variants_callset WHERE processed_sample_id='4000'").toString(), "DRAGEN");
		S_EQUAL(db.getValue("SELECT caller_version FROM small_variants_callset WHERE processed_sample_id='4000'").toString(), "3.0.2");
		S_EQUAL(db.getValue("SELECT call_date FROM small_variants_callset WHERE processed_sample_id='4000'").toDate().toString(Qt::ISODate), "2018-11-08");

		//check if PubMed ids are imported
		Variant var = Variant(Chromosome("chrX"), 155255024, 155255024, "C", "T");
		QString var_id = db.variantId(var);
		QStringList pubmed_ids = db.pubmedIds(var_id);
		pubmed_ids.sort();
		S_EQUAL(pubmed_ids.at(0), "12345678");
		S_EQUAL(pubmed_ids.at(1), "87654321");

		//check mosaic flag is imported
		I_EQUAL(db.getValue("SELECT mosaic FROM detected_variant WHERE variant_id=" + var_id).toInt(), 1);
		Variant var2 = Variant(Chromosome("chrX"), 155253718, 155253718, "G", "A");
		QString var2_id = db.variantId(var2);
		I_EQUAL(db.getValue("SELECT mosaic FROM detected_variant WHERE variant_id=" + var2_id).toInt(), 0);
	}

	void sv_default_import()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 35);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 8);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 36);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 6);
		count = db.getValue("SELECT count(*) FROM sv_callset").toInt();
		I_EQUAL(count, 1);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line94.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line94.log", TESTDATA("data_out/NGSDAddVariantsGermline_out5.log"));
	}


	void sv_failed_reimport()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//re-import SVs for same sample without "-force" (no change)
		EXECUTE_FAIL("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in_empty.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 35);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 8);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 36);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 6);
		count = db.getValue("SELECT count(*) FROM sv_callset").toInt();
		I_EQUAL(count, 1);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line125.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line125.log", TESTDATA("data_out/NGSDAddVariantsGermline_out6.log"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line128.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line128.log", TESTDATA("data_out/NGSDAddVariantsGermline_out7.log"));
	}


	void sv_forced_reimport()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//re-import SVs for same sample with "-force" (overwrite)
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv_force -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in_empty.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_callset").toInt();
		I_EQUAL(count, 1);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line161.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line161.log", TESTDATA("data_out/NGSDAddVariantsGermline_out8.log"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line164.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line164.log", TESTDATA("data_out/NGSDAddVariantsGermline_out9.log"));
	}

	void import_with_existing_report_config()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_report_config.sql"));

		//try to import variants
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -cnv_force -cnv " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.tsv"));
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv_force -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in3.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM variant").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM cnv").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 0);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line198.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line198.log", TESTDATA("data_out/NGSDAddVariantsGermline_out10.log"));
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line199.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line199.log", TESTDATA("data_out/NGSDAddVariantsGermline_out11.log"));
	}

	void sv_longread_import()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -sv " + TESTDATA("data_in/NGSDAddVariantsGermline_in4.bedpe"));

		//check db content
		int count = db.getValue("SELECT count(*) FROM sv_deletion").toInt();
		I_EQUAL(count, 123);
		count = db.getValue("SELECT count(*) FROM sv_duplication").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM sv_insertion").toInt();
		I_EQUAL(count, 142);
		count = db.getValue("SELECT count(*) FROM sv_inversion").toInt();
		I_EQUAL(count, 1);
		count = db.getValue("SELECT count(*) FROM sv_translocation").toInt();
		I_EQUAL(count, 3);
		count = db.getValue("SELECT count(*) FROM sv_callset").toInt();
		I_EQUAL(count, 1);

		//check log
		REMOVE_LINES("out/NGSDAddVariantsGermline_Test_line233.log", QRegExp("^filename:"));
		COMPARE_FILES("out/NGSDAddVariantsGermline_Test_line233.log", TESTDATA("data_out/NGSDAddVariantsGermline_Test_line233.log"));
	}

	void re_import()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsGermline_init.sql"));

		//check db is empty
		int count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype").toInt();
		I_EQUAL(count, 0);

		//check import of ExpansionHunter
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -re " + TESTDATA("data_in/NGSDAddVariantsGermline_in5.vcf"));
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype").toInt();
		I_EQUAL(count, 84);
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype WHERE allele2 IS NULL").toInt();
		I_EQUAL(count, 11);
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype WHERE allele1 > 30").toInt();
		I_EQUAL(count, 2);
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype WHERE allele2 > 30").toInt();
		I_EQUAL(count, 4);
		count = db.getValue("SELECT count(*) FROM re_callset").toInt();
		I_EQUAL(count, 1);
		S_EQUAL(db.getValue("SELECT caller FROM re_callset").toString(), "ExpansionHunter");
		S_EQUAL(db.getValue("SELECT caller_version FROM re_callset").toString(), "v5.0.0");
		S_EQUAL(db.getValue("SELECT call_date FROM re_callset").toDateTime().toString(Qt::ISODate), "2024-04-16T00:00:00");

		//check import of Straglr
		EXECUTE("NGSDAddVariantsGermline", "-test -debug -no_time -ps NA12878_45 -re_force -re " + TESTDATA("data_in/NGSDAddVariantsGermline_in6.vcf"));
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype").toInt();
		I_EQUAL(count, 30);
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype WHERE allele2 IS NULL").toInt();
		I_EQUAL(count, 0);
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype WHERE allele1 >= 30").toInt();
		I_EQUAL(count, 3);
		count = db.getValue("SELECT count(*) FROM repeat_expansion_genotype WHERE allele2 >= 30").toInt();
		I_EQUAL(count, 3);
		count = db.getValue("SELECT count(*) FROM re_callset").toInt();
		I_EQUAL(count, 1);
		S_EQUAL(db.getValue("SELECT caller FROM re_callset").toString(), "Straglr");
		S_EQUAL(db.getValue("SELECT caller_version FROM re_callset").toString(), "V1.5.0");
		S_EQUAL(db.getValue("SELECT call_date FROM re_callset").toDateTime().toString(Qt::ISODate), "2024-06-06T00:00:00");
	}

};


