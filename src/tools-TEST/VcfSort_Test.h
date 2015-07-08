#include "TestFramework.h"

class VcfSort_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("VcfSort", "-in " + QFINDTESTDATA("data_in/VcfSort_in1.vcf") + " -out out/VcfSort_out1.vcf");
		TFW::comareFiles("out/VcfSort_out1.vcf", QFINDTESTDATA("data_out/VcfSort_out1.vcf"));
	}

	void test_02()
	{
		TFW_EXEC("VcfSort", "-in " + QFINDTESTDATA("data_in/VcfSort_in2.vcf") + " -out out/VcfSort_out2.vcf -fai " + QFINDTESTDATA("data_in/hg19.fa.fai"));
		TFW::comareFiles("out/VcfSort_out2.vcf", QFINDTESTDATA("data_out/VcfSort_out2.vcf"));
	}

};

TFW_DECLARE(VcfSort_Test)

