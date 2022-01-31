#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"
#include "VcfFile.h"
#include <iostream>

TEST_CLASS(VcfAnnotateFromBigWig_Test)
{
Q_OBJECT
private slots:

	void test01()
	{
		EXECUTE("VcfAnnotateFromBigWig", "-in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in.vcf") + " -out out/VcfAnnotateFromBigWig_out1.vcf -name PHYLOP -desc phylop_value -bw " + TESTDATA("../cppNGS-TEST/data_in/BigWigReader_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out1.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out1.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out1.vcf"))
	}

	void test01_multithreaded()
	{
		EXECUTE("VcfAnnotateFromBigWig", "-threads 5 -block_size 25 -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in.vcf") + " -out out/VcfAnnotateFromBigWig_out1.vcf -name PHYLOP -desc phylop_value -bw " + TESTDATA("../cppNGS-TEST/data_in/BigWigReader_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out1.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out1.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out1.vcf"))
	}

	void test02()
	{
		EXECUTE("VcfAnnotateFromBigWig", "-in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in2.vcf") + " -out out/VcfAnnotateFromBigWig_out2.vcf -name PHYLOP -desc phylop_value -bw " + TESTDATA("../cppNGS-TEST/data_in/BigWigReader_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out2.vcf");
		COMPARE_FILES("out/VcfAnnotateFromBigWig_out2.vcf", TESTDATA("data_out/VcfAnnotateFromBigWig_out2.vcf"))
	}
};
