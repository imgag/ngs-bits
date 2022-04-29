#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(HgvsToVcf_Test)
{
Q_OBJECT
private slots:
	
	void test1()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in1.tsv") + " -out out/HgvsToVcf_out1.vcf" + " -ref " + ref_file);
		REMOVE_LINES("out/HgvsToVcf_out1.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/HgvsToVcf_out1.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/HgvsToVcf_out1.vcf", TESTDATA("data_out/HgvsToVcf_out1.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out1.vcf");
	}

	void no_header()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in2.tsv") + " -out out/HgvsToVcf_out2.vcf" + " -ref " + ref_file);
		REMOVE_LINES("out/HgvsToVcf_out2.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/HgvsToVcf_out2.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/HgvsToVcf_out2.vcf", TESTDATA("data_out/HgvsToVcf_out2.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out2.vcf");
	}

	void rename_column()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in2.tsv") + " -out out/HgvsToVcf_out3.vcf" + " -input_info_field test_name -ref " + ref_file);
		REMOVE_LINES("out/HgvsToVcf_out3.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/HgvsToVcf_out3.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/HgvsToVcf_out3.vcf", TESTDATA("data_out/HgvsToVcf_out3.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out3.vcf");
	}

	void refseq_transcripts()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in4.tsv") + " -out out/HgvsToVcf_out4.vcf" + " -ref " + ref_file);
		REMOVE_LINES("out/HgvsToVcf_out4.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/HgvsToVcf_out4.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/HgvsToVcf_out4.vcf", TESTDATA("data_out/HgvsToVcf_out4.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out4.vcf");
	}

	void ccds_transcripts()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in5.tsv") + " -out out/HgvsToVcf_out5.vcf" + " -ref " + ref_file);
		REMOVE_LINES("out/HgvsToVcf_out5.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/HgvsToVcf_out5.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/HgvsToVcf_out5.vcf", TESTDATA("data_out/HgvsToVcf_out5.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out5.vcf");
	}


};


