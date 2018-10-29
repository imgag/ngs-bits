#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfFilter_TEST)
{
Q_OBJECT
private slots:
    void region_file()
    {
        // there are no overlapping regions
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out01.vcf" + " -reg " + TESTDATA("data_in/SampleSimilarity_roi.bed"));
        COMPARE_FILES("out/VcfFilter_out01.vcf", TESTDATA("data_out/VcfFilter_out01.vcf"));
        VCF_IS_VALID("out/VcfFilter_out01.vcf");
    }

    void region_string()
    {
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out02.vcf" + " -reg chr1:27687466-62739198");
        COMPARE_FILES("out/VcfFilter_out02.vcf", TESTDATA("data_out/VcfFilter_out02.vcf"));
        VCF_IS_VALID("out/VcfFilter_out02.vcf");
    }

    void variant_type()
    {
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out03.vcf" + " -variant_type snp");
        COMPARE_FILES("out/VcfFilter_out03.vcf", TESTDATA("data_out/VcfFilter_out03.vcf"));
        VCF_IS_VALID("out/VcfFilter_out03.vcf");
    }

    void id_regex()
    {
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out04.vcf" + " -id rs2");
        COMPARE_FILES("out/VcfFilter_out04.vcf", TESTDATA("data_out/VcfFilter_out04.vcf"));
        VCF_IS_VALID("out/VcfFilter_out04.vcf");
    }

    void quality()
    {
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out05.vcf" + " -qual 3000");
        COMPARE_FILES("out/VcfFilter_out05.vcf", TESTDATA("data_out/VcfFilter_out05.vcf"));
        VCF_IS_VALID("out/VcfFilter_out05.vcf");
    }

    void filter_empty()
    {
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out06.vcf" + " -filter_empty");
        COMPARE_FILES("out/VcfFilter_out06.vcf", TESTDATA("data_out/VcfFilter_out06.vcf"));
        VCF_IS_VALID("out/VcfFilter_out06.vcf");
    }

    void filter_regex()
    {
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out07.vcf" + " -filter off-target");
        COMPARE_FILES("out/VcfFilter_out07.vcf", TESTDATA("data_out/VcfFilter_out07.vcf"));
        VCF_IS_VALID("out/VcfFilter_out07.vcf");
    }

    void info_complex()
    {
        /*EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out08.vcf" + " -info_filter \"DP > 100;AO > 5\"");
        COMPARE_FILES("out/VcfFilter_out08.vcf", TESTDATA("data_out/VcfFilter_out08.vcf"));
        VCF_IS_VALID("out/VcfFilter_out08.vcf");*/
    }

    void sample_complex()
    {
        /*EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out09.vcf" + " -info_filter \"GL >= -200\"");
        COMPARE_FILES("out/VcfFilter_out09.vcf", TESTDATA("data_out/VcfFilter_out09.vcf"));
        VCF_IS_VALID("out/VcfFilter_out09.vcf");*/
    }
};
