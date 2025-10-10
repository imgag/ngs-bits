#include "TestFrameworkNGS.h"

TEST_CLASS(VcfFilter_Test)
{
private:

    TEST_METHOD(region_file)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out01.vcf" + " -reg " + TESTDATA("data_in/VcfFilter_roi.bed"));
        COMPARE_FILES("out/VcfFilter_out01.vcf", TESTDATA("data_out/VcfFilter_out01.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out01.vcf");
    }

    TEST_METHOD(region_string)
    {
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out02.vcf" + " -reg chr1:27687466-62728838,chr1:62728861-62739198");
        COMPARE_FILES("out/VcfFilter_out02.vcf", TESTDATA("data_out/VcfFilter_out02.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out02.vcf");
    }

    TEST_METHOD(variant_type)
    {
		SKIP_IF_NO_HG38_GENOME();

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out03.vcf" + " -variant_type snp");
        COMPARE_FILES("out/VcfFilter_out03.vcf", TESTDATA("data_out/VcfFilter_out03.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out03.vcf");
    }

	TEST_METHOD(id)
    {
		SKIP_IF_NO_HG38_GENOME();

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out04.vcf" + " -id rs2");
        COMPARE_FILES("out/VcfFilter_out04.vcf", TESTDATA("data_out/VcfFilter_out04.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out04.vcf");
    }

    TEST_METHOD(quality)
    {
		SKIP_IF_NO_HG38_GENOME();

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out05.vcf" + " -qual 3000");
        COMPARE_FILES("out/VcfFilter_out05.vcf", TESTDATA("data_out/VcfFilter_out05.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out05.vcf");
    }

    TEST_METHOD(filter_empty)
    {
		SKIP_IF_NO_HG38_GENOME();

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out06.vcf" + " -filter_empty");
        COMPARE_FILES("out/VcfFilter_out06.vcf", TESTDATA("data_out/VcfFilter_out06.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out06.vcf");
    }

	TEST_METHOD(filter)
    {
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out07.vcf" + " -filter off-target");
		COMPARE_FILES("out/VcfFilter_out07.vcf", TESTDATA("data_out/VcfFilter_out07.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out07.vcf");
    }

	TEST_METHOD(filter_exclude)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out12.vcf" + " -filter_exclude off-target");
		COMPARE_FILES("out/VcfFilter_out12.vcf", TESTDATA("data_out/VcfFilter_out06.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out12.vcf");
	}

	TEST_METHOD(filter_filters)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out13.vcf" + " -filter off-target -filter_exclude test");
		COMPARE_FILES("out/VcfFilter_out13.vcf", TESTDATA("data_out/VcfFilter_out13.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out13.vcf");
	}

	TEST_METHOD(info)
    {
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out08.vcf" + " -info DP%20>%20100;AO%20>%205");
        COMPARE_FILES("out/VcfFilter_out08.vcf", TESTDATA("data_out/VcfFilter_out08.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out08.vcf");
    }

	TEST_METHOD(sample)
    {
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out09.vcf" + " -sample GT%20is%201|1;DP%20>%20200");
        COMPARE_FILES("out/VcfFilter_out09.vcf", TESTDATA("data_out/VcfFilter_out09.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out09.vcf");
    }

	TEST_METHOD(multisample_sample)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in02.vcf") + " -out out/VcfFilter_out10.vcf" + " -sample GT%20is%201|1;DP%20>%20200");
		COMPARE_FILES("out/VcfFilter_out10.vcf", TESTDATA("data_out/VcfFilter_out10.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out10.vcf");
	}

	TEST_METHOD(multisample_sample_onematch)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in02.vcf") + " -out out/VcfFilter_out11.vcf" + " -sample GT%20is%201|1;DP%20>%20200 -sample_one_match");
        COMPARE_FILES("out/VcfFilter_out11.vcf", TESTDATA("data_out/VcfFilter_out11.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out11.vcf");
    }

	TEST_METHOD(remove_invalid)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in03.vcf") + " -out out/VcfFilter_out14.vcf" + " -remove_invalid");
		COMPARE_FILES("out/VcfFilter_out14.vcf", TESTDATA("data_out/VcfFilter_out14.vcf"));
		VCF_IS_VALID("out/VcfFilter_out14.vcf");
	}

	TEST_METHOD(remove_non_ref)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in03.vcf") + " -out out/VcfFilter_out15.vcf" + " -remove_non_ref");
		COMPARE_FILES("out/VcfFilter_out15.vcf", TESTDATA("data_out/VcfFilter_out15.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out15.vcf");
	}

	TEST_METHOD(filter_clear)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in03.vcf") + " -out out/VcfFilter_out16.vcf" + " -remove_non_ref -filter_clear");
		COMPARE_FILES("out/VcfFilter_out16.vcf", TESTDATA("data_out/VcfFilter_out16.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out16.vcf");
	}

/************************************ BUGS ************************************/

	TEST_METHOD(bugfix_tab_before_column_returned)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_bug01.vcf") + " -out out/VcfFilter_bug01.vcf" + " -sample GT%20not%20./0");
		COMPARE_FILES("out/VcfFilter_bug01.vcf", TESTDATA("data_out/VcfFilter_bug01.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_bug01.vcf");
	}
};
