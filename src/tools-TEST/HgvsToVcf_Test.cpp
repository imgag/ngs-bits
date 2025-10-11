#include "TestFrameworkNGS.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(HgvsToVcf_Test)
{
private:
	
	TEST_METHOD(test1)
	{
		SKIP_IF_NO_HG38_GENOME();
		SKIP_IF_NO_TEST_NGSD();

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/HgvsToVcf_init.sql"));

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in1.tsv") + " -out out/HgvsToVcf_out1.vcf -test" + " -ref " + ref_file);
        REMOVE_LINES("out/HgvsToVcf_out1.vcf", QRegularExpression("^##fileDate="));
        REMOVE_LINES("out/HgvsToVcf_out1.vcf", QRegularExpression("^##reference="));
        REMOVE_LINES("out/HgvsToVcf_out1.vcf", QRegularExpression("^##INFO=<ID=count,Number"));
		COMPARE_FILES("out/HgvsToVcf_out1.vcf", TESTDATA("data_out/HgvsToVcf_out1.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out1.vcf");
	}

	TEST_METHOD(no_header)
	{
        SKIP_IF_NO_HG38_GENOME();
		SKIP_IF_NO_TEST_NGSD();

		QString ref_file = Settings::string("reference_genome", true);

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/HgvsToVcf_init.sql"));

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in2.tsv") + " -out out/HgvsToVcf_out2.vcf -test" + " -ref " + ref_file);
        REMOVE_LINES("out/HgvsToVcf_out2.vcf", QRegularExpression("^##fileDate="));
        REMOVE_LINES("out/HgvsToVcf_out2.vcf", QRegularExpression("^##reference="));
		COMPARE_FILES("out/HgvsToVcf_out2.vcf", TESTDATA("data_out/HgvsToVcf_out2.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out2.vcf");
	}

	TEST_METHOD(rename_column)
	{
		SKIP_IF_NO_HG38_GENOME();
		SKIP_IF_NO_TEST_NGSD();

		QString ref_file = Settings::string("reference_genome", true);

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/HgvsToVcf_init.sql"));

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in2.tsv") + " -out out/HgvsToVcf_out3.vcf -test" + " -input_info_field test_name -ref " + ref_file);
        REMOVE_LINES("out/HgvsToVcf_out3.vcf", QRegularExpression("^##fileDate="));
        REMOVE_LINES("out/HgvsToVcf_out3.vcf", QRegularExpression("^##reference="));
		COMPARE_FILES("out/HgvsToVcf_out3.vcf", TESTDATA("data_out/HgvsToVcf_out3.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out3.vcf");
	}

	TEST_METHOD(refseq_transcripts)
	{
		SKIP_IF_NO_HG38_GENOME();
		SKIP_IF_NO_TEST_NGSD();

		QString ref_file = Settings::string("reference_genome", true);

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/HgvsToVcf_init.sql"));

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in4.tsv") + " -out out/HgvsToVcf_out4.vcf -test" + " -ref " + ref_file);
        REMOVE_LINES("out/HgvsToVcf_out4.vcf", QRegularExpression("^##fileDate="));
        REMOVE_LINES("out/HgvsToVcf_out4.vcf", QRegularExpression("^##reference="));
        REMOVE_LINES("out/HgvsToVcf_out4.vcf", QRegularExpression("^##INFO=<ID=count,Number"));
		COMPARE_FILES("out/HgvsToVcf_out4.vcf", TESTDATA("data_out/HgvsToVcf_out4.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out4.vcf");
	}

	TEST_METHOD(ccds_transcripts)
	{
		SKIP_IF_NO_HG38_GENOME();
		SKIP_IF_NO_TEST_NGSD();

		QString ref_file = Settings::string("reference_genome", true);

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/HgvsToVcf_init.sql"));

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in5.tsv") + " -out out/HgvsToVcf_out5.vcf -test" + " -ref " + ref_file);
        REMOVE_LINES("out/HgvsToVcf_out5.vcf", QRegularExpression("^##fileDate="));
        REMOVE_LINES("out/HgvsToVcf_out5.vcf", QRegularExpression("^##reference="));
        REMOVE_LINES("out/HgvsToVcf_out5.vcf", QRegularExpression("^##INFO=<ID=count,Number"));
		COMPARE_FILES("out/HgvsToVcf_out5.vcf", TESTDATA("data_out/HgvsToVcf_out5.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out5.vcf");
	}
};


