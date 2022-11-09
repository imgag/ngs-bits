#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfFilter_TEST)
{
Q_OBJECT
private slots:

    void region_file()
    {
        // there are no overlapping regions
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out01.vcf" + " -reg " + TESTDATA("data_in/VcfFilter_roi.bed"));
        COMPARE_FILES("out/VcfFilter_out01.vcf", TESTDATA("data_out/VcfFilter_out01.vcf"));
        VCF_IS_VALID("out/VcfFilter_out01.vcf");
    }

    void region_string()
    {
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out02.vcf" + " -reg chr1:27687466-62728838,chr1:62728861-62739198");
        COMPARE_FILES("out/VcfFilter_out02.vcf", TESTDATA("data_out/VcfFilter_out02.vcf"));
        VCF_IS_VALID("out/VcfFilter_out02.vcf");
    }

    void variant_type()
    {
        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out03.vcf" + " -variant_type snp");
        COMPARE_FILES("out/VcfFilter_out03.vcf", TESTDATA("data_out/VcfFilter_out03.vcf"));
        VCF_IS_VALID("out/VcfFilter_out03.vcf");
    }

	void id()
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

	void filter()
    {
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out07.vcf" + " -filter off-target");
		COMPARE_FILES("out/VcfFilter_out07.vcf", TESTDATA("data_out/VcfFilter_out07.vcf"));
		VCF_IS_VALID("out/VcfFilter_out07.vcf");
    }

	void filter_exclude()
	{
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out12.vcf" + " -filter_exclude off-target");
		COMPARE_FILES("out/VcfFilter_out12.vcf", TESTDATA("data_out/VcfFilter_out06.vcf"));
		VCF_IS_VALID("out/VcfFilter_out12.vcf");
	}

	void filter_filters()
	{
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out13.vcf" + " -filter off-target -filter_exclude test");
		COMPARE_FILES("out/VcfFilter_out13.vcf", TESTDATA("data_out/VcfFilter_out13.vcf"));
		VCF_IS_VALID("out/VcfFilter_out13.vcf");
	}

	void info()
    {
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out08.vcf" + " -info DP%20>%20100;AO%20>%205");
        COMPARE_FILES("out/VcfFilter_out08.vcf", TESTDATA("data_out/VcfFilter_out08.vcf"));
		VCF_IS_VALID("out/VcfFilter_out08.vcf");
    }

	void sample()
    {
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out09.vcf" + " -sample GT%20is%201|1;DP%20>%20200");
        COMPARE_FILES("out/VcfFilter_out09.vcf", TESTDATA("data_out/VcfFilter_out09.vcf"));
		VCF_IS_VALID("out/VcfFilter_out09.vcf");
    }

	void multisample_sample()
	{
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in02.vcf") + " -out out/VcfFilter_out10.vcf" + " -sample GT%20is%201|1;DP%20>%20200");
		COMPARE_FILES("out/VcfFilter_out10.vcf", TESTDATA("data_out/VcfFilter_out10.vcf"));
		VCF_IS_VALID("out/VcfFilter_out10.vcf");
	}

	void multisample_sample_onematch()
	{
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in02.vcf") + " -out out/VcfFilter_out11.vcf" + " -sample GT%20is%201|1;DP%20>%20200 -sample_one_match");
        COMPARE_FILES("out/VcfFilter_out11.vcf", TESTDATA("data_out/VcfFilter_out11.vcf"));
        VCF_IS_VALID("out/VcfFilter_out11.vcf");
    }

/************************************ BUGS ************************************/

	void bugfix_tab_before_column_returned()
	{
		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_bug01.vcf") + " -out out/VcfFilter_bug01.vcf" + " -sample GT%20not%20./0");
		COMPARE_FILES("out/VcfFilter_bug01.vcf", TESTDATA("data_out/VcfFilter_bug01.vcf"));
		VCF_IS_VALID("out/VcfFilter_bug01.vcf");
	}
};
