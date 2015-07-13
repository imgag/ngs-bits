#include "TestFramework.h"

TEST_CLASS(VariantQC_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("VariantQC", "-in " + TESTDATA("../tools-TEST/data_in/VariantQC_in1.vcf") + " -out out/VariantQC_test01_out.txt -txt");
		COMPARE_FILES("out/VariantQC_test01_out.txt", TESTDATA("data_out/VariantQC_test01_out.txt"));
	}
	
	void test_02()
	{
		EXECUTE("VariantQC", "-in " + TESTDATA("../tools-TEST/data_in/VariantQC_in1.vcf") + " -out out/VariantQC_test02_out.qcML");
		REMOVE_LINES("out/VariantQC_test02_out.qcML", QRegExp("creation "));
		COMPARE_FILES("out/VariantQC_test02_out.qcML", TESTDATA("data_out/VariantQC_test02_out.qcML"));
	}

};


