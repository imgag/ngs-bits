#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfSort_Test)
{
Q_OBJECT
private slots:
	
	void fake_data()
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in1.vcf") + " -out out/VcfSort_out1.vcf");
		COMPARE_FILES("out/VcfSort_out1.vcf", TESTDATA("data_out/VcfSort_out1.vcf"));
		//VCF_IS_VALID not applied because this is fake data with invalid chromosomes/coordinates
	}

	void fake_data_withFAI()
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in2.vcf") + " -out out/VcfSort_out2.vcf -fai " + TESTDATA("data_in/hg19.fa.fai"));
		COMPARE_FILES("out/VcfSort_out2.vcf", TESTDATA("data_out/VcfSort_out2.vcf"));
		//VCF_IS_VALID not applied because this is fake data with invalid chromosomes/coordinates
	}

	void real_data_withFAI()
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in3.vcf") + " -out out/VcfSort_out3.vcf -fai " + TESTDATA("data_in/hg19.fa.fai"));
		COMPARE_FILES("out/VcfSort_out3.vcf", TESTDATA("data_out/VcfSort_out3.vcf"));
		VCF_IS_VALID("out/VcfSort_out3.vcf")
	}

	void bug_GT_not_first_format_field()
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in4.vcf") + " -out out/VcfSort_out4.vcf");
		COMPARE_FILES("out/VcfSort_out4.vcf", TESTDATA("data_out/VcfSort_out4.vcf"));
		VCF_IS_VALID("out/VcfSort_out4.vcf")
	}

	void remove_unused_contigs()
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in3.vcf") + " -remove_unused_contigs -out out/VcfSort_out5.vcf");
		COMPARE_FILES("out/VcfSort_out5.vcf", TESTDATA("data_out/VcfSort_out5.vcf"));
		VCF_IS_VALID("out/VcfSort_out5.vcf")
	}
};


