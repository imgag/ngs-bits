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

	void big_test()
	{
//		QTime timer;
//		timer.start();
//		EXECUTE("VcfAnnotateFromBigWig", "-threads 5 -in /mnt/users/ahott1a1/NA12878_45_var.vcf -out out/VcfAnnotateFromBigWig_out1.vcf -name PHYLOP -desc phylop_value_as_given_by_vep -bw /mnt/storage2/GRCh38/share/data/dbs/phyloP/hg38.phyloP100way.bw");
//		std::cout << "Pure Annotation took: " << timer.elapsed()/1000 << "s\n";
//		VCF_IS_VALID("out/VcfAnnotateFromBigWig_out1.vcf");

		QSharedPointer<QFile> out = Helper::openFileForWriting("/mnt/users/ahott1a1/VcfAnnotateFrombigWig_test_out.txt");
		VcfFile test = VcfFile();
		test.load("/mnt/users/ahott1a1/VcfAnnotateFromBigWig_out_NA12878_45.vcf");
		VcfFile ref = VcfFile();
		ref.load("/mnt/users/ahott1a1/NA1287_45_Test_vcf-vep3.vcf");

		I_EQUAL(test.count(), ref.count());



		int i_phylop = ref.vcfHeader().vepIndexByName("PHYLOP", false);
		for (int i=0; i<test.count(); i++)
		{

			float expected = ref[i].vepAnnotations(i_phylop)[0].toDouble();
			float actual = test[i].info("PHYLOP", true).toFloat();

			if (std::abs(expected -actual) > 0.000001)
			{
				QByteArray line = "Mutation \t" + QByteArray::number(i) + ": \t" + ref[i].chr().str() + "\t" + QByteArray::number(ref[i].start()) + "\t" + ref[i].ref() + "\t" + QByteArray::number(expected) + "\t" + QByteArray::number(actual) + "\n";
				out->write(line);
			}

			//F_EQUAL(actual, expected);
		}
	}
};
