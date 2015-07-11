#include "TestFramework.h"

class BedSubtract_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("BedSubtract", "-in " + QFINDTESTDATA("data_in/BedSubtract_in1.bed") + " -in2 " + QFINDTESTDATA("data_in/BedSubtract_in2.bed") + " -out out/BedSubtract_test01_out.bed");
		TFW::comareFiles("out/BedSubtract_test01_out.bed", QFINDTESTDATA("data_out/BedSubtract_test01_out.bed"));
	}
	
	void test_02()
	{
		TFW_EXEC("BedSubtract", "-in " + QFINDTESTDATA("data_in/BedSubtract_in2.bed") + " -in2 " + QFINDTESTDATA("data_in/BedSubtract_in1.bed") + " -out out/BedSubtract_test02_out.bed");
		TFW::comareFiles("out/BedSubtract_test02_out.bed", QFINDTESTDATA("data_out/BedSubtract_test02_out.bed"));
	}
	
};

TFW_DECLARE(BedSubtract_Test)


