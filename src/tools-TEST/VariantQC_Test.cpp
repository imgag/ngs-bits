#include "TestFramework.h"

TEST_CLASS(VariantQC_Test)
{
private:
	
	TEST_METHOD(test_txt_nofilter)
	{
		EXECUTE("VariantQC", "-in " + TESTDATA("data_in/VariantQC_in1.vcf") + " -out out/VariantQC_test01_out.txt -txt -ignore_filter");
		COMPARE_FILES("out/VariantQC_test01_out.txt", TESTDATA("data_out/VariantQC_test01_out.txt"));
	}

	TEST_METHOD(test_qcML_nofilter)
	{
		EXECUTE("VariantQC", "-in " + TESTDATA("data_in/VariantQC_in1.vcf") + " -out out/VariantQC_test02_out.qcML -ignore_filter");
        REMOVE_LINES("out/VariantQC_test02_out.qcML", QRegularExpression("creation "));
		COMPARE_FILES("out/VariantQC_test02_out.qcML", TESTDATA("data_out/VariantQC_test02_out.qcML"));
	}

	TEST_METHOD(test_qcML_filter)
	{
		EXECUTE("VariantQC", "-in " + TESTDATA("data_in/VariantQC_in1.vcf") + " -out out/VariantQC_test03_out.qcML");
        REMOVE_LINES("out/VariantQC_test03_out.qcML", QRegularExpression("creation "));
		COMPARE_FILES("out/VariantQC_test03_out.qcML", TESTDATA("data_out/VariantQC_test03_out.qcML"));
	}

	TEST_METHOD(test_qcML_long_read)
	{
		EXECUTE("VariantQC", "-long_read -in " + TESTDATA("data_in/VariantQC_in2.vcf") + " -out out/VariantQC_test04_out.qcML -phasing_bed out/VariantQC_test04_out.bed");
        REMOVE_LINES("out/VariantQC_test04_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/VariantQC_test04_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/VariantQC_test04_out.qcML", TESTDATA("data_out/VariantQC_test04_out.qcML"));
		COMPARE_FILES("out/VariantQC_test04_out.bed", TESTDATA("data_out/VariantQC_test04_out.bed"));
	}

};


