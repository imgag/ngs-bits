#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"
#include "VcfFile.h"
#include <iostream>

TEST_CLASS(VcfAnnotateFromBigWig_Test)
{
private:

	TEST_METHOD(test01)
	{
		EXECUTE("VcfAnnotateFromBigWig", "-mode max -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in.vcf") + " -out out/VcfAnnotateFromBigWig_out1.vcf -name PHYLOP -bw " + TESTDATA("data_in/VcfAnnotateFromBigWig_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out1.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out1.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out1.vcf"))
	}

	TEST_METHOD(test01_multithreaded)
	{
		EXECUTE("VcfAnnotateFromBigWig", "-mode max -threads 5 -block_size 25 -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in.vcf") + " -out out/VcfAnnotateFromBigWig_out1.vcf -name PHYLOP -bw " + TESTDATA("data_in/VcfAnnotateFromBigWig_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out1.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out1.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out1.vcf"))
	}

	TEST_METHOD(mode_max)
	{
		EXECUTE("VcfAnnotateFromBigWig", "-mode max -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in2.vcf") + " -out out/VcfAnnotateFromBigWig_out2.vcf -name PHYLOP -bw " + TESTDATA("data_in/VcfAnnotateFromBigWig_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out2.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out2.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out2.vcf"))
	}

	TEST_METHOD(mode_min)
	{
		EXECUTE("VcfAnnotateFromBigWig", "-mode min -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in2.vcf") + " -out out/VcfAnnotateFromBigWig_out3.vcf -name PHYLOP -bw " + TESTDATA("data_in/VcfAnnotateFromBigWig_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out2.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out3.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out3.vcf"))
	}

	TEST_METHOD(mode_avg)
	{
		EXECUTE("VcfAnnotateFromBigWig", "-mode avg -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in2.vcf") + " -out out/VcfAnnotateFromBigWig_out4.vcf -name PHYLOP -bw " + TESTDATA("data_in/VcfAnnotateFromBigWig_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out2.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out4.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out4.vcf"))
	}

	TEST_METHOD(mode_none)
	{
		EXECUTE("VcfAnnotateFromBigWig", "-mode none -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in2.vcf") + " -out out/VcfAnnotateFromBigWig_out5.vcf -name PHYLOP -bw " + TESTDATA("data_in/VcfAnnotateFromBigWig_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out2.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out5.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out5.vcf"))
	}
};
