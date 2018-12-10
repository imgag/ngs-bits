#ifndef VCFBREAKCOMPLEXVARIANTS_TEST_H
#define VCFBREAKCOMPLEXVARIANTS_TEST_H
#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfBreakComplexVariants_TEST)
{
Q_OBJECT
private slots:
	void test_vt_cases()
	{
		// Test cases from VT, see https://github.com/atks/vt/tree/master/test/decompose_blocksub
		EXECUTE("VcfBreakComplexVariants", "-in " + TESTDATA("data_in/VcfBreakComplexVariants_in01.vcf") + " -out out/VcfBreakComplexVariants_out01.vcf");
		COMPARE_FILES("out/VcfBreakComplexVariants_out01.vcf", TESTDATA("data_out/VcfBreakComplexVariants_out01.vcf"));
		VCF_IS_VALID("out/VcfBreakComplexVariants_out01.vcf");
	}
};


#endif // VCFBREAKCOMPLEXVARIANTS_TEST_H
