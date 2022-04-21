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

        EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " +
                TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out1.vcf");
        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out1.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out1.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out1.vcf"));
    }

    //use a different tag (with CSQ already present)
    void different_tag()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in2.vcf") + " -gff " +
                TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out2.vcf" +
                " -tag CSQ_2");
        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out2.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out2.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out2.vcf"));
    }

    //replace a consequence string that is already present (with tag CSQ)
    void replace_csq()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in3.vcf") + " -gff " +
                TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out3.vcf" +
                " -tag CSQ");
        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out3.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out3.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out3.vcf"));
    }

    //reduce the maximal distance for transcripts to be considered
    void define_dist_to_trans()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " +
                TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out4.vcf" +
                " -dist-to-trans 1000");
        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out4.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out4.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out4.vcf"));
    }

    //reduce the size of the splice region (intron at both ends and exon)
    void reduce_splice_region()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " +
                TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out5.vcf" +
                " -splice-region-ex 1 -splice-region-in-5 4 -splice-region-in-3 4");
        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out5.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out5.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out5.vcf"));
    }

    //increase the size of the splice region (intron at both ends and exon)
    void increase_splice_region()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in1.vcf") + " -gff " +
                TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out6.vcf" +
                " -splice-region-ex 5 -splice-region-in-5 25 -splice-region-in-3 25");
        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out6.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out6.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out6.vcf"));
    }
};
