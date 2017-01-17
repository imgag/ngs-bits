#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VariantAnnotateStrand_Test)
{
Q_OBJECT
private slots:

	//Test with name and depth arguments
	void test_01()
	{
		EXECUTE("VariantAnnotateStrand", "-bam " + TESTDATA("data_in/VariantAnnotateStrand_in1.bam") + " -vcf " + TESTDATA("data_in/VariantAnnotateStrand_in1.vcf") + " -out out/VariantAnnotateStrand_out1.vcf");
		COMPARE_FILES("out/VariantAnnotateStrand_out1.vcf", TESTDATA("data_out/VariantAnnotateStrand_out1.vcf"));
	}
};


