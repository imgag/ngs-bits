#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(MappingQC_Test)
{
private:
	
	TEST_METHOD(roi_amplicon)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -build hg19 -out out/MappingQC_test01_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test01_out.qcML", TESTDATA("data_out/MappingQC_test01_out.qcML"));
	}

	TEST_METHOD(roi_amplicon_mapq0)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -build hg19 -out out/MappingQC_test06_out.qcML -min_mapq 0 -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test06_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test06_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test06_out.qcML", TESTDATA("data_out/MappingQC_test06_out.qcML"));
	}

	TEST_METHOD(roi_shotgun_txt)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -build hg19 -out out/MappingQC_test02_out.txt -txt -ref " + ref_file);
		COMPARE_FILES("out/MappingQC_test02_out.txt", TESTDATA("data_out/MappingQC_test02_out.txt"));
	}
	
	TEST_METHOD(roi_shotgun_singleend)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -build hg19 -out out/MappingQC_test03_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test03_out.qcML", TESTDATA("data_out/MappingQC_test03_out.qcML"));
	}
	
	TEST_METHOD(wgs_shotgun)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -wgs -build hg19 -out out/MappingQC_test04_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test04_out.qcML", TESTDATA("data_out/MappingQC_test04_out.qcML"));
	}

	TEST_METHOD(wgs_shotgun_singleend)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -wgs -build hg19 -out out/MappingQC_test05_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test05_out.qcML", TESTDATA("data_out/MappingQC_test05_out.qcML"));
	}

	TEST_METHOD(wgs_with_raw_read_qc)
	{
		//to test coverage and GC/AT dropout statistics
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in5.bam") + " -wgs -build hg38" + " -out out/MappingQC_test10_out.qcML -read_qc out/MappingQC_test11_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test10_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test10_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test10_out.qcML", TESTDATA("data_out/MappingQC_test10_out.qcML"));
        REMOVE_LINES("out/MappingQC_test11_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test11_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test11_out.qcML", TESTDATA("data_out/MappingQC_test11_out.qcML"));
	}

    TEST_METHOD(rna_pairedend)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in3.bam") + " -rna -build hg19 -out out/MappingQC_test07_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test07_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test07_out.qcML", QRegularExpression("<binary>"));
        COMPARE_FILES("out/MappingQC_test07_out.qcML", TESTDATA("data_out/MappingQC_test07_out.qcML"));
    }
	TEST_METHOD(cfdna)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in4.bam") + " -roi " + TESTDATA("data_in/MappingQC_in3.bed") + " -cfdna -build hg19 -out out/MappingQC_test08_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test08_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test08_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test08_out.qcML", TESTDATA("data_out/MappingQC_test08_out.qcML"));
	}

	TEST_METHOD(somatic_custom)
	{
		QString ref_file = Settings::string("reference_genome_hg19", true);
		if (ref_file=="") SKIP("Test needs the reference genome HG19!");

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -somatic_custom_bed " + TESTDATA("data_in/MappingQC_in2_custom_subpanel.bed") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -build hg19 -out out/MappingQC_test09_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test09_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test09_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test09_out.qcML", TESTDATA("data_out/MappingQC_test09_out.qcML"));
	}

	TEST_METHOD(lrgs_with_raw_read_qc)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		EXECUTE("MappingQC", "-long_read -in " + TESTDATA("data_in/MappingQC_in6.bam") + " -wgs -build hg38" + " -out out/MappingQC_test12_out.qcML -read_qc out/MappingQC_test13_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test12_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test12_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test12_out.qcML", TESTDATA("data_out/MappingQC_test12_out.qcML"));
        REMOVE_LINES("out/MappingQC_test13_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test13_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test13_out.qcML", TESTDATA("data_out/MappingQC_test13_out.qcML"));
	}


};
