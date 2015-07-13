#include "TestFramework.h"

TEST_CLASS(VcfSort_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in1.vcf") + " -out out/VcfSort_out1.vcf");
		COMPARE_FILES("out/VcfSort_out1.vcf", TESTDATA("data_out/VcfSort_out1.vcf"));
	}

	void test_02()
	{
		EXECUTE("VcfSort", "-in " + TESTDATA("data_in/VcfSort_in2.vcf") + " -out out/VcfSort_out2.vcf -fai " + TESTDATA("data_in/hg19.fa.fai"));
		COMPARE_FILES("out/VcfSort_out2.vcf", TESTDATA("data_out/VcfSort_out2.vcf"));
	}

};


