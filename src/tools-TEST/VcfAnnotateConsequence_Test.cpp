#include "TestFrameworkNGS.h"
#include "Settings.h"


TEST_CLASS(VcfAnnotateConsequence_Test)
{
private:

    //test with default parameters
    TEST_METHOD(default_params)
    {
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out1.vcf -splice_region_in5 8 -splice_region_in3 8");

        COMPARE_FILES("out/VcfAnnotateConsequence_out1.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out1.vcf"));
    }

    //use a different tag (with CSQ already present)
    TEST_METHOD(different_tag)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in2.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out2.vcf -tag CSQ_2 -splice_region_in5 8 -splice_region_in3 8");

        COMPARE_FILES("out/VcfAnnotateConsequence_out2.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out2.vcf"));
    }

    //replace a consequence string that is already present (with tag CSQ)
    TEST_METHOD(replace_csq)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in2.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out3.vcf" + " -tag CSQ -all -splice_region_in5 8 -splice_region_in3 8");

        COMPARE_FILES("out/VcfAnnotateConsequence_out3.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out3.vcf"));
    }

    //reduce the maximal distance for transcripts to be considered
    TEST_METHOD(define_dist_to_trans)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out4.vcf" + " -max_dist_to_trans 1000 -splice_region_in5 8 -splice_region_in3 8");

        COMPARE_FILES("out/VcfAnnotateConsequence_out4.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out4.vcf"));
    }

    //reduce the size of the splice region (intron at both ends and exon)
    TEST_METHOD(reduce_splice_region)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out5.vcf" + " -splice_region_ex 1 -splice_region_in5 4 -splice_region_in3 4");

        COMPARE_FILES("out/VcfAnnotateConsequence_out5.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out5.vcf"));
    }

    //increase the size of the splice region (intron at both ends and exon)
    TEST_METHOD(increase_splice_region)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out6.vcf" + " -splice_region_ex 5 -splice_region_in5 25 -splice_region_in3 25");

        COMPARE_FILES("out/VcfAnnotateConsequence_out6.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out6.vcf"));
    }

	//RefSeq source
	TEST_METHOD(refseq)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts_refseq.gff3.gz") + " -out out/VcfAnnotateConsequence_out7.vcf" + " -source refseq");

		COMPARE_FILES("out/VcfAnnotateConsequence_out7.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out7.vcf"));
	}

	TEST_METHOD(multithreaded)
	{
		SKIP_IF_NO_HG38_GENOME();

		for (int t=1; t<=4; ++t)
		{
			QString out_file = "out/VcfAnnotateConsequence_out1_"+QByteArray::number(t)+"threads.vcf";
			EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out "+out_file+" -splice_region_in5 8 -splice_region_in3 8 -block_size 20 -threads " + QByteArray::number(t));
			COMPARE_FILES(out_file, TESTDATA("data_out/VcfAnnotateConsequence_out1.vcf"));
		}
	}

};
