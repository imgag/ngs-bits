#include "TestFrameworkNGS.h"

TEST_CLASS(VcfAnnotateMaxEntScan_Test)
{
private:
	
	TEST_METHOD(with_tags_decimals_minscore)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateMaxEntScan", "-in " + TESTDATA("data_in/VcfAnnotateMaxEntScan_in1.vcf") + " -out out/VcfAnnotateMaxEntScan_out1.vcf" + " -gff " + TESTDATA("data_in/VcfAnnotateMaxEntScan_transcripts.gff3") + " -swa -tag mes -tag_swa mes_swa -min_score 0 -decimals 1");
		COMPARE_FILES("out/VcfAnnotateMaxEntScan_out1.vcf", TESTDATA("data_out/VcfAnnotateMaxEntScan_out1.vcf"));
		VCF_IS_VALID("out/VcfAnnotateMaxEntScan_out1.vcf");
	}

	TEST_METHOD(splicing_variants)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateMaxEntScan", "-in " + TESTDATA("data_in/VcfAnnotateMaxEntScan_in2.vcf") + " -out out/VcfAnnotateMaxEntScan_out2.vcf" + " -gff " + TESTDATA("data_in/VcfAnnotateMaxEntScan_transcripts_2.gff3") + " -swa");
		COMPARE_FILES("out/VcfAnnotateMaxEntScan_out2.vcf", TESTDATA("data_out/VcfAnnotateMaxEntScan_out2.vcf"));
		VCF_IS_VALID("out/VcfAnnotateMaxEntScan_out2.vcf");
	}


	//multi-thread test
	TEST_METHOD(test_multithread)
	{
		SKIP_IF_NO_HG38_GENOME();

		for (int i=2; i<=8; ++i)
		{
			QString suffix = QString::number(i) + "threads";

			EXECUTE("VcfAnnotateMaxEntScan", "-in " + TESTDATA("data_in/VcfAnnotateMaxEntScan_in2.vcf") + " -out out/VcfAnnotateMaxEntScan_out_" + suffix +".vcf" + " -gff " + TESTDATA("data_in/VcfAnnotateMaxEntScan_transcripts_2.gff3") + " -swa -block_size 30 -threads " + QString::number(i) );
			COMPARE_FILES("out/VcfAnnotateMaxEntScan_out_" + suffix +".vcf", TESTDATA("data_out/VcfAnnotateMaxEntScan_out2.vcf"));
			VCF_IS_VALID("out/VcfAnnotateMaxEntScan_out_" + suffix +".vcf");
		}
	}

};
