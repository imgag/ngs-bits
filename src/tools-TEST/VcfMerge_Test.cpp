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

    TEST_METHOD(trio_with_recalling)
    {
        SKIP_IF_NO_HG38_GENOME();

        QString ref_file = Settings::string("reference_genome", true);

        EXECUTE("VcfMerge", "-trio -in " + TESTDATA("data_in/VcfMerge_SR_FB.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DR.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DV.vcf.gz") + " -out out/VcfMerge_out3.vcf -bam " + TESTDATA("data_in/VcfMerge.cram") + " " + TESTDATA("data_in/VcfMerge.cram") + " " + TESTDATA("data_in/VcfMerge.cram"));
        REMOVE_LINES("out/VcfMerge_out3.vcf", QRegularExpression("fileDate"));
        COMPARE_FILES("out/VcfMerge_out3.vcf", TESTDATA("data_out/VcfMerge_out3.vcf"));
        VCF_IS_VALID("out/VcfMerge_out3.vcf");
	}

	TEST_METHOD(trio_no_special_calls_qual20)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfMerge", "-trio -in " + TESTDATA("data_in/VcfMerge_SR_FB.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DR.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DV.vcf.gz") + " -out out/VcfMerge_out4.vcf -no_special_calls -min_qual 20");
		REMOVE_LINES("out/VcfMerge_out4.vcf", QRegularExpression("fileDate"));
		COMPARE_FILES("out/VcfMerge_out4.vcf", TESTDATA("data_out/VcfMerge_out4.vcf"));
		VCF_IS_VALID("out/VcfMerge_out4.vcf");
	}

	TEST_METHOD(trio_with_recalling_no_gt_correction)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);

		EXECUTE("VcfMerge", "-trio -no_genotype_correction -in " + TESTDATA("data_in/VcfMerge_SR_FB.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DR.vcf.gz") + " " + TESTDATA("data_in/VcfMerge_SR_DV.vcf.gz") + " -out out/VcfMerge_out5.vcf -bam " + TESTDATA("data_in/VcfMerge.cram") + " " + TESTDATA("data_in/VcfMerge.cram") + " " + TESTDATA("data_in/VcfMerge.cram"));
		REMOVE_LINES("out/VcfMerge_out5.vcf", QRegularExpression("fileDate"));
		COMPARE_FILES("out/VcfMerge_out5.vcf", TESTDATA("data_out/VcfMerge_out5.vcf"));
		VCF_IS_VALID("out/VcfMerge_out5.vcf");
	}
};
