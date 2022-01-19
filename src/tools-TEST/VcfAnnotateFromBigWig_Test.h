#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"
#include "VcfFile.h"

TEST_CLASS(VcfAnnotateFromBigWig_Test)
{
Q_OBJECT
private slots:

	void test01()
	{
		EXECUTE("VcfAnnotateFromBigWig", "-in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in.vcf") + " -out out/VcfAnnotateFromBigWig_out1.vcf -name PHYLOP -desc phylop_value_as_given_by_vep -bw " + TESTDATA("../cppNGS-TEST/data_in/BigWigReader_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out1.vcf");

		VcfFile test = VcfFile();
		test.load("out/VcfAnnotateFromBigWig_out1.vcf");
		VcfFile ref = VcfFile();
		ref.load(TESTDATA("../cppNGS-TEST/data_in/BigWigReader_vep1.vcf"));

		I_EQUAL(test.count(), ref.count());

		int i_phylop = ref.vcfHeader().vepIndexByName("PHYLOP", false);
		for (int i=0; i<test.count(); i++)
		{
			float expected = ref[i].vepAnnotations(i_phylop)[0].toDouble();
			float actual = test[i].info("PHYLOP", true).toFloat();

			F_EQUAL(actual, expected);
		}
	}

	void test01_multithreaded()
	{
		EXECUTE("VcfAnnotateFromBigWig", "-threads 5 -in " + TESTDATA("data_in/VcfAnnotateFromBigWig_in.vcf") + " -out out/VcfAnnotateFromBigWig_out1.vcf -name PHYLOP -desc phylop_value_as_given_by_vep -bw " + TESTDATA("../cppNGS-TEST/data_in/BigWigReader_phyloP_chr1_part.bw"));
		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out1.vcf");

		VcfFile test = VcfFile();
		test.load("out/VcfAnnotateFromBigWig_out1.vcf");
		VcfFile ref = VcfFile();
		ref.load(TESTDATA("../cppNGS-TEST/data_in/BigWigReader_vep1.vcf"));

		I_EQUAL(test.count(), ref.count());

		int i_phylop = ref.vcfHeader().vepIndexByName("PHYLOP", false);
		for (int i=0; i<test.count(); i++)
		{
			float expected = ref[i].vepAnnotations(i_phylop)[0].toDouble();
			float actual = test[i].info("PHYLOP", true).toFloat();

			F_EQUAL(actual, expected);
		}
	}
};
