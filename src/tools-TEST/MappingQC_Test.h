#include "TestFramework.h"

TEST_CLASS(MappingQC_Test)
{
Q_OBJECT
private slots:
	
	void roi_amplicon()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/MappingQC_test01_out.qcML");
		REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test01_out.qcML", TESTDATA("data_out/MappingQC_test01_out.qcML"));
	}
	
	void roi_shotgun_3exons_txt()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test02_out.txt -txt -3exons");
		COMPARE_FILES("out/MappingQC_test02_out.txt", TESTDATA("data_out/MappingQC_test02_out.txt"));
	}
	
	void roi_shotgun_singleend()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test03_out.qcML");
		REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test03_out.qcML", TESTDATA("data_out/MappingQC_test03_out.qcML"));
	}
	
	void wgs_shotgun()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -wgs -out out/MappingQC_test04_out.qcML");
		REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test04_out.qcML", TESTDATA("data_out/MappingQC_test04_out.qcML"));
	}

	void wgs_shotgun_singleend()
	{
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -wgs -out out/MappingQC_test05_out.qcML");
		REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test05_out.qcML", TESTDATA("data_out/MappingQC_test05_out.qcML"));
	}
};
