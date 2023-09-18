#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfAnnotateMaxEntScan_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateMaxEntScan", "-in " + TESTDATA("data_in/VcfAnnotateMaxEntScan_in1.vcf") + " -out out/VcfAnnotateMaxEntScan_out1.vcf" + " -gff " + TESTDATA("data_in/VcfAnnotateMaxEntScan_transcripts.gff3") + " -swa");
		COMPARE_FILES("out/VcfAnnotateMaxEntScan_out1.vcf", TESTDATA("data_out/VcfAnnotateMaxEntScan_out1.vcf"));
		VCF_IS_VALID("out/VcfAnnotateMaxEntScan_out1.vcf");
	}

	void splicing_variants()
	{
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateMaxEntScan", "-in " + TESTDATA("data_in/VcfAnnotateMaxEntScan_in2.vcf") + " -out out/VcfAnnotateMaxEntScan_out2.vcf" + " -gff " + TESTDATA("data_in/VcfAnnotateMaxEntScan_transcripts_2.gff3") + " -swa");
		COMPARE_FILES("out/VcfAnnotateMaxEntScan_out2.vcf", TESTDATA("data_out/VcfAnnotateMaxEntScan_out2.vcf"));
		VCF_IS_VALID("out/VcfAnnotateMaxEntScan_out2.vcf");
	}


	//multi-thread test
	void test_multithread()
	{
		QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
		for (int i=2; i<=8; ++i)
		{
			QString suffix = QString::number(i) + "threads";

			EXECUTE("VcfAnnotateMaxEntScan", "-in " + TESTDATA("data_in/VcfAnnotateMaxEntScan_in2.vcf") + " -out out/VcfAnnotateMaxEntScan_out_" + suffix +".vcf" + " -gff " + TESTDATA("data_in/VcfAnnotateMaxEntScan_transcripts_2.gff3") + " -swa -block_size 30 -threads " + QString::number(i) );
			COMPARE_FILES("out/VcfAnnotateMaxEntScan_out_" + suffix +".vcf", TESTDATA("data_out/VcfAnnotateMaxEntScan_out2.vcf"));
			VCF_IS_VALID("out/VcfAnnotateMaxEntScan_out_" + suffix +".vcf");
		}
	}

};
