#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfFilter_TEST)
{
Q_OBJECT
private slots:

    void region_file()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out01.vcf" + " -reg " + TESTDATA("data_in/VcfFilter_roi.bed"));
        COMPARE_FILES("out/VcfFilter_out01.vcf", TESTDATA("data_out/VcfFilter_out01.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out01.vcf");
    }

    void region_string()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out02.vcf" + " -reg chr1:27687466-62728838,chr1:62728861-62739198");
        COMPARE_FILES("out/VcfFilter_out02.vcf", TESTDATA("data_out/VcfFilter_out02.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out02.vcf");
    }

    void variant_type()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out03.vcf" + " -variant_type snp");
        COMPARE_FILES("out/VcfFilter_out03.vcf", TESTDATA("data_out/VcfFilter_out03.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out03.vcf");
    }

	void id()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out04.vcf" + " -id rs2");
        COMPARE_FILES("out/VcfFilter_out04.vcf", TESTDATA("data_out/VcfFilter_out04.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out04.vcf");
    }

    void quality()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out05.vcf" + " -qual 3000");
        COMPARE_FILES("out/VcfFilter_out05.vcf", TESTDATA("data_out/VcfFilter_out05.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out05.vcf");
    }

    void filter_empty()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out06.vcf" + " -filter_empty");
        COMPARE_FILES("out/VcfFilter_out06.vcf", TESTDATA("data_out/VcfFilter_out06.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out06.vcf");
    }

	void filter()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out07.vcf" + " -filter off-target");
		COMPARE_FILES("out/VcfFilter_out07.vcf", TESTDATA("data_out/VcfFilter_out07.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out07.vcf");
    }

	void filter_exclude()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out12.vcf" + " -filter_exclude off-target");
		COMPARE_FILES("out/VcfFilter_out12.vcf", TESTDATA("data_out/VcfFilter_out06.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out12.vcf");
	}

	void filter_filters()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out13.vcf" + " -filter off-target -filter_exclude test");
		COMPARE_FILES("out/VcfFilter_out13.vcf", TESTDATA("data_out/VcfFilter_out13.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out13.vcf");
	}

	void info()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out08.vcf" + " -info DP%20>%20100;AO%20>%205");
        COMPARE_FILES("out/VcfFilter_out08.vcf", TESTDATA("data_out/VcfFilter_out08.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out08.vcf");
    }

	void sample()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in01.vcf") + " -out out/VcfFilter_out09.vcf" + " -sample GT%20is%201|1;DP%20>%20200");
        COMPARE_FILES("out/VcfFilter_out09.vcf", TESTDATA("data_out/VcfFilter_out09.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out09.vcf");
    }

	void multisample_sample()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in02.vcf") + " -out out/VcfFilter_out10.vcf" + " -sample GT%20is%201|1;DP%20>%20200");
		COMPARE_FILES("out/VcfFilter_out10.vcf", TESTDATA("data_out/VcfFilter_out10.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out10.vcf");
	}

	void multisample_sample_onematch()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in02.vcf") + " -out out/VcfFilter_out11.vcf" + " -sample GT%20is%201|1;DP%20>%20200 -sample_one_match");
        COMPARE_FILES("out/VcfFilter_out11.vcf", TESTDATA("data_out/VcfFilter_out11.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out11.vcf");
    }

	void remove_invalid()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in03.vcf") + " -out out/VcfFilter_out14.vcf" + " -remove_invalid");
		COMPARE_FILES("out/VcfFilter_out14.vcf", TESTDATA("data_out/VcfFilter_out14.vcf"));
		VCF_IS_VALID("out/VcfFilter_out14.vcf");
	}

	void remove_non_ref()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in03.vcf") + " -out out/VcfFilter_out15.vcf" + " -remove_non_ref");
		COMPARE_FILES("out/VcfFilter_out15.vcf", TESTDATA("data_out/VcfFilter_out15.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out15.vcf");
	}

	void filter_clear()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_in03.vcf") + " -out out/VcfFilter_out16.vcf" + " -remove_non_ref -filter_clear");
		COMPARE_FILES("out/VcfFilter_out16.vcf", TESTDATA("data_out/VcfFilter_out16.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_out16.vcf");
	}

/************************************ BUGS ************************************/

	void bugfix_tab_before_column_returned()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfFilter", "-in " + TESTDATA("data_in/VcfFilter_bug01.vcf") + " -out out/VcfFilter_bug01.vcf" + " -sample GT%20not%20./0");
		COMPARE_FILES("out/VcfFilter_bug01.vcf", TESTDATA("data_out/VcfFilter_bug01.vcf"));
		VCF_IS_VALID_HG19("out/VcfFilter_bug01.vcf");
	}
};
