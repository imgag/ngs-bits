#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(MappingQC_Test)
{
Q_OBJECT
private slots:
	
	void roi_amplicon()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/MappingQC_test01_out.qcML -ref " + ref_file);
		REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test01_out.qcML", TESTDATA("data_out/MappingQC_test01_out.qcML"));
	}

	void roi_amplicon_mapq0()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -out out/MappingQC_test06_out.qcML -min_mapq 0 -ref " + ref_file);
		REMOVE_LINES("out/MappingQC_test06_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test06_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test06_out.qcML", TESTDATA("data_out/MappingQC_test06_out.qcML"));
	}

	void roi_shotgun_txt()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test02_out.txt -txt -ref " + ref_file);
		COMPARE_FILES("out/MappingQC_test02_out.txt", TESTDATA("data_out/MappingQC_test02_out.txt"));
	}
	
	void roi_shotgun_singleend()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -out out/MappingQC_test03_out.qcML -ref " + ref_file);
		REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test03_out.qcML", TESTDATA("data_out/MappingQC_test03_out.qcML"));
	}
	
	void wgs_shotgun()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -wgs -out out/MappingQC_test04_out.qcML -ref " + ref_file);
		REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test04_out.qcML", TESTDATA("data_out/MappingQC_test04_out.qcML"));
	}

	void wgs_shotgun_singleend()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -wgs -out out/MappingQC_test05_out.qcML -ref " + ref_file);
		REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test05_out.qcML", TESTDATA("data_out/MappingQC_test05_out.qcML"));
	}
    void rna_pairedend()
    {
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in3.bam") + " -rna -out out/MappingQC_test07_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test07_out.qcML", QRegExp("creation "));
        REMOVE_LINES("out/MappingQC_test07_out.qcML", QRegExp("<binary>"));
        COMPARE_FILES("out/MappingQC_test07_out.qcML", TESTDATA("data_out/MappingQC_test07_out.qcML"));
    }
	void cfdna()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in4.bam") + " -roi " + TESTDATA("data_in/MappingQC_in3.bed") + " -cfdna -out out/MappingQC_test08_out.qcML -ref " + ref_file);
		REMOVE_LINES("out/MappingQC_test08_out.qcML", QRegExp("creation "));
		REMOVE_LINES("out/MappingQC_test08_out.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/MappingQC_test08_out.qcML", TESTDATA("data_out/MappingQC_test08_out.qcML"));
	}
};
