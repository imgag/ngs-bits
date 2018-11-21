#ifndef VCFBREAKCOMPLEXVARIANTS_TEST_H
#define VCFBREAKCOMPLEXVARIANTS_TEST_H
#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfBreakComplexVariants_TEST)
{
Q_OBJECT
private slots:
	void even_length()
	{
		EXECUTE("VcfBreakComplexVariants", "-in " + TESTDATA("data_in/VcfBreakComplexVariants_in01.vcf") + " -out out/VcfBreakComplexVariants_out01.vcf");
		COMPARE_FILES("out/VcfBreakComplexVariants_in01.vcf", TESTDATA("data_out/VcfBreakComplexVariants_out01.vcf"));
		VCF_IS_VALID("out/VcfAnnotateFromBed_out01.vcf");
	}

	void uneven_length()
	{
		EXECUTE("VcfBreakComplexVariants", "-in " + TESTDATA("data_in/VcfBreakComplexVariants_in02.vcf") + " -out out/VcfBreakComplexVariants_out02.vcf");
		COMPARE_FILES("out/VcfBreakComplexVariants_in02.vcf", TESTDATA("data_out/VcfBreakComplexVariants_out02.vcf"));
		VCF_IS_VALID("out/VcfAnnotateFromBed_out02.vcf");
	}

	void phased_even_length()
	{
		EXECUTE("VcfBreakComplexVariants", "-in " + TESTDATA("data_in/VcfBreakComplexVariants_in03.vcf") + " -out out/VcfBreakComplexVariants_out03.vcf");
		COMPARE_FILES("out/VcfBreakComplexVariants_in03.vcf", TESTDATA("data_out/VcfBreakComplexVariants_out03.vcf"));
		VCF_IS_VALID("out/VcfAnnotateFromBed_out03.vcf");
	}
};


#endif // VCFBREAKCOMPLEXVARIANTS_TEST_H
