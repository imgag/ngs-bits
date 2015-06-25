#include "../TestFramework.h"

class VariantQC_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("VariantQC", "-in " + QFINDTESTDATA("../tools-TEST/data_in/VariantQC_in1.vcf") + " -out out/VariantQC_test01_out.txt -txt");
		TFW::comareFiles("out/VariantQC_test01_out.txt", QFINDTESTDATA("data_out/VariantQC_test01_out.txt"));
	}
	
	void test_02()
	{
		TFW_EXEC("VariantQC", "-in " + QFINDTESTDATA("../tools-TEST/data_in/VariantQC_in1.vcf") + " -out out/VariantQC_test02_out.qcML");
		TFW::removeLinesContaining("out/VariantQC_test02_out.qcML", "creation ");
		TFW::comareFiles("out/VariantQC_test02_out.qcML", QFINDTESTDATA("data_out/VariantQC_test02_out.qcML"));
	}

};

TFW_DECLARE(VariantQC_Test)

