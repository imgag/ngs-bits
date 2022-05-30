#include "TestFramework.h"
#include "Settings.h"


TEST_CLASS(VcfAnnotateConsequence_Test)
{
Q_OBJECT
private slots:

    //test with default parameters
    void default_params()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out1.vcf -splice_region_in5 8 -splice_region_in3 8");

		IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out1.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out1.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out1.vcf"));
    }

    //use a different tag (with CSQ already present)
    void different_tag()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in2.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out2.vcf -tag CSQ_2 -splice_region_in5 8 -splice_region_in3 8");

		IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out2.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out2.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out2.vcf"));
    }

    //replace a consequence string that is already present (with tag CSQ)
    void replace_csq()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in2.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out3.vcf" + " -tag CSQ -all -splice_region_in5 8 -splice_region_in3 8");

		IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out3.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out3.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out3.vcf"));
    }

    //reduce the maximal distance for transcripts to be considered
    void define_dist_to_trans()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out4.vcf" + " -max_dist_to_trans 1000 -splice_region_in5 8 -splice_region_in3 8");

        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out4.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out4.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out4.vcf"));
    }

    //reduce the size of the splice region (intron at both ends and exon)
    void reduce_splice_region()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out5.vcf" + " -splice_region_ex 1 -splice_region_in5 4 -splice_region_in3 4");

		IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out5.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out5.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out5.vcf"));
    }

    //increase the size of the splice region (intron at both ends and exon)
    void increase_splice_region()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " + TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out6.vcf" + " -splice_region_ex 5 -splice_region_in5 25 -splice_region_in3 25");

		IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out6.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out6.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out6.vcf"));
    }
};
