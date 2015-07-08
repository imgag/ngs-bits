#include "TestFramework.h"

class MappingQC_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("MappingQC", "-in " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + QFINDTESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/MappingQC_test01_out.txt -txt -3exons");
		TFW::comareFiles("out/MappingQC_test01_out.txt", QFINDTESTDATA("data_out/MappingQC_test01_out.txt"));
	}
	
	void test_02()
	{
		TFW_EXEC("MappingQC", "-in " + QFINDTESTDATA("data_in/MappingQC_in2.bam") + " -roi " + QFINDTESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test02_out.txt -txt -3exons");
		TFW::comareFiles("out/MappingQC_test02_out.txt", QFINDTESTDATA("data_out/MappingQC_test02_out.txt"));
	}
	
	void test_03()
	{
		TFW_EXEC("MappingQC", "-in " + QFINDTESTDATA("data_in/MappingQC_in2.bam") + " -roi " + QFINDTESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test03_out.qcML -3exons");
		TFW::removeLinesContaining("out/MappingQC_test03_out.qcML", "creation ");
		TFW::removeLinesContaining("out/MappingQC_test03_out.qcML", "<binary>");
		TFW::comareFiles("out/MappingQC_test03_out.qcML", QFINDTESTDATA("data_out/MappingQC_test03_out.qcML"));
	}
	
	void test_04()
	{
		TFW_EXEC("MappingQC", "-in " + QFINDTESTDATA("data_in/MappingQC_in2.bam") + " -wgs hg19 -out out/MappingQC_test04_out.txt -txt");
	    TFW::comareFiles("out/MappingQC_test04_out.txt", QFINDTESTDATA("data_out/MappingQC_test04_out.txt"));
	}

};

TFW_DECLARE(MappingQC_Test)

