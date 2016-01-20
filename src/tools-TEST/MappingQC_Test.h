#include "TestFramework.h"

TEST_CLASS(MappingQC_Test)
{
Q_OBJECT
private slots:
	
	void txt_roi_3exons_panel()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/MappingQC_test01_out.txt -txt -3exons");
		COMPARE_FILES("out/MappingQC_test01_out.txt", TESTDATA("data_out/MappingQC_test01_out.txt"));
	}
	
	void txt_roi_3exons()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test02_out.txt -txt -3exons");
		COMPARE_FILES("out/MappingQC_test02_out.txt", TESTDATA("data_out/MappingQC_test02_out.txt"));
	}
	
	void qcml_roi_3exons()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test03_out.qcML -3exons");
		REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test03_out.qcML", TESTDATA("data_out/MappingQC_test03_out.qcML"));
	}
	
	void txt_wgs()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -wgs 3095693983 -out out/MappingQC_test04_out.txt -txt");
	    COMPARE_FILES("out/MappingQC_test04_out.txt", TESTDATA("data_out/MappingQC_test04_out.txt"));
	}

};
