#include "TestFramework.h"

class VariantFilterRegions_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("VariantFilterRegions", "-in " + QFINDTESTDATA("data_in/VariantFilterRegions_in.tsv") + " -reg " + QFINDTESTDATA("data_in/VariantFilterRegions_in.bed") + " -out out/VariantFilterRegions_out1.tsv");
		TFW::comareFiles("out/VariantFilterRegions_out1.tsv", QFINDTESTDATA("data_out/VariantFilterRegions_out1.tsv"));
	}
	
};

TFW_DECLARE(VariantFilterRegions_Test)

