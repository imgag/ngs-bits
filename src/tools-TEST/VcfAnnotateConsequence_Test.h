#include "TestFramework.h"
#include "Settings.h"


TEST_CLASS(VcfAnnotateConsequence_Test)
{
Q_OBJECT
private slots:

    void default_params()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfAnnotateConsequence", "-in " + TESTDATA("data_in/VcfAnnotateConsequence_in.vcf") + " -gff " +
                TESTDATA("data_in/VcfAnnotateConsequence_transcripts.gff3") + " -out out/VcfAnnotateConsequence_out.vcf");
        IS_TRUE(QFile::exists("out/VcfAnnotateConsequence_out.vcf"));
        COMPARE_FILES("out/VcfAnnotateConsequence_out.vcf", TESTDATA("data_out/VcfAnnotateConsequence_out.vcf"));
    }
};
