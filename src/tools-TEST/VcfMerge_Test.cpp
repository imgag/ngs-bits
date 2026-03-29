#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfMerge_Test)
{
private:

    TEST_METHOD(multi_sample)
	{
        SKIP_IF_NO_HG38_GENOME();

        QString ref_file = Settings::string("reference_genome", true);

        EXECUTE("VcfMerge", "-in " + TESTDATA("data_in/VcfMerge_SR_FB.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DR.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DV.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_LR_ONT_CL.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_LR_PacBio_DV.vcf.gz") + " -out out/VcfMerge_out1.vcf");
        REMOVE_LINES("out/VcfMerge_out1.vcf", QRegularExpression("fileDate"));
        COMPARE_FILES("out/VcfMerge_out1.vcf", TESTDATA("data_out/VcfMerge_out1.vcf"));
        VCF_IS_VALID("out/VcfMerge_out1.vcf");
	}

    TEST_METHOD(trio)
    {
        SKIP_IF_NO_HG38_GENOME();

        QString ref_file = Settings::string("reference_genome", true);

        EXECUTE("VcfMerge", "-trio -in " + TESTDATA("data_in/VcfMerge_SR_FB.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DR.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DV.vcf.gz") + " -out out/VcfMerge_out2.vcf");
        REMOVE_LINES("out/VcfMerge_out2.vcf", QRegularExpression("fileDate"));
        COMPARE_FILES("out/VcfMerge_out2.vcf", TESTDATA("data_out/VcfMerge_out2.vcf"));
        VCF_IS_VALID("out/VcfMerge_out2.vcf");
    }
};
