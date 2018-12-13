#ifndef VCFBREAKCOMPLEX_TEST_H
#define VCFBREAKCOMPLEX_TEST_H
#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfBreakComplex_TEST)
{
Q_OBJECT
private slots:
	void test_vt_cases()
	{
		// Test cases from VT, see https://github.com/atks/vt/tree/master/test/decompose_blocksub
                EXECUTE("VcfBreakComplex", "-in " + TESTDATA("data_in/VcfBreakComplex_in01.vcf") + " -out out/VcfBreakComplex_out01.vcf");
                COMPARE_FILES("out/VcfBreakComplex_out01.vcf", TESTDATA("data_out/VcfBreakComplex_out01.vcf"));
                VCF_IS_VALID("out/VcfBreakComplex_out01.vcf");
	}

	void test_simple_snp_korell()
	{
                EXECUTE("VcfBreakComplex", "-in " + TESTDATA("data_in/VcfBreakComplex_in02.vcf") + " -out out/VcfBreakComplex_out02.vcf");
                COMPARE_FILES("out/VcfBreakComplex_out02.vcf", TESTDATA("data_out/VcfBreakComplex_out02.vcf"));
                VCF_IS_VALID("out/VcfBreakComplex_out02.vcf");
	}

	void test_no_tag()
	{
                EXECUTE("VcfBreakComplex", "-in " + TESTDATA("data_in/VcfBreakComplex_in02.vcf") + " -out out/VcfBreakComplex_out03.vcf -no_tag");
                COMPARE_FILES("out/VcfBreakComplex_out03.vcf", TESTDATA("data_out/VcfBreakComplex_out03.vcf"));
                VCF_IS_VALID("out/VcfBreakComplex_out03.vcf");
	}

	void test_keep_mnps()
	{
                EXECUTE("VcfBreakComplex", "-in " + TESTDATA("data_in/VcfBreakComplex_in04.vcf") + " -out out/VcfBreakComplex_out04.vcf -keep_mnps");
                COMPARE_FILES("out/VcfBreakComplex_out04.vcf", TESTDATA("data_out/VcfBreakComplex_out04.vcf"));
                VCF_IS_VALID("out/VcfBreakComplex_out04.vcf");
	}
};


#endif // VCFBREAKCOMPLEX_TEST_H
