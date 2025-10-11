#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(MappingQC_Test)
{
private:
	
	TEST_METHOD(roi_amplicon)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -build hg19 -out out/MappingQC_test01_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test01_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test01_out.qcML", TESTDATA("data_out/MappingQC_test01_out.qcML"));
	}

	TEST_METHOD(roi_amplicon_mapq0)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -roi " + TESTDATA("../cppNGS-TEST/data_in/panel.bed") + " -build hg19 -out out/MappingQC_test06_out.qcML -min_mapq 0 -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test06_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test06_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test06_out.qcML", TESTDATA("data_out/MappingQC_test06_out.qcML"));
	}

	TEST_METHOD(roi_shotgun_txt)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -build hg19 -out out/MappingQC_test02_out.txt -txt -ref " + ref_file);
		COMPARE_FILES("out/MappingQC_test02_out.txt", TESTDATA("data_out/MappingQC_test02_out.txt"));
	}
	
	TEST_METHOD(roi_shotgun_singleend)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -build hg19 -out out/MappingQC_test03_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test03_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test03_out.qcML", TESTDATA("data_out/MappingQC_test03_out.qcML"));
	}
	
	TEST_METHOD(wgs_shotgun)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -wgs -build hg19 -out out/MappingQC_test04_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test04_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test04_out.qcML", TESTDATA("data_out/MappingQC_test04_out.qcML"));
	}

	TEST_METHOD(wgs_shotgun_singleend)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in1.bam") + " -wgs -build hg19 -out out/MappingQC_test05_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test05_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test05_out.qcML", TESTDATA("data_out/MappingQC_test05_out.qcML"));
	}

	TEST_METHOD(wgs_with_raw_read_qc)
	{
		SKIP_IF_NO_HG38_GENOME();

		//to test coverage and GC/AT dropout statistics
		QString ref_file = Settings::string("reference_genome", true);
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
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in3.bam") + " -rna -build hg19 -out out/MappingQC_test07_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test07_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test07_out.qcML", QRegularExpression("<binary>"));
        COMPARE_FILES("out/MappingQC_test07_out.qcML", TESTDATA("data_out/MappingQC_test07_out.qcML"));
    }
	TEST_METHOD(cfdna)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in4.bam") + " -roi " + TESTDATA("data_in/MappingQC_in3.bed") + " -cfdna -build hg19 -out out/MappingQC_test08_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test08_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test08_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test08_out.qcML", TESTDATA("data_out/MappingQC_test08_out.qcML"));
	}

	TEST_METHOD(somatic_custom)
	{
		SKIP_IF_NO_HG19_GENOME();

		QString ref_file = Settings::string("reference_genome_hg19", true);

		EXECUTE("MappingQC", "-in " + TESTDATA("data_in/MappingQC_in2.bam") + " -somatic_custom_bed " + TESTDATA("data_in/MappingQC_in2_custom_subpanel.bed") + " -roi " + TESTDATA("data_in/MappingQC_in2.bed") + " -build hg19 -out out/MappingQC_test09_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test09_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test09_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test09_out.qcML", TESTDATA("data_out/MappingQC_test09_out.qcML"));
	}

	TEST_METHOD(lrgs_with_raw_read_qc)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);
		EXECUTE("MappingQC", "-long_read -in " + TESTDATA("data_in/MappingQC_in6.bam") + " -wgs -build hg38" + " -out out/MappingQC_test12_out.qcML -read_qc out/MappingQC_test13_out.qcML -ref " + ref_file);
        REMOVE_LINES("out/MappingQC_test12_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test12_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test12_out.qcML", TESTDATA("data_out/MappingQC_test12_out.qcML"));
        REMOVE_LINES("out/MappingQC_test13_out.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/MappingQC_test13_out.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/MappingQC_test13_out.qcML", TESTDATA("data_out/MappingQC_test13_out.qcML"));
	}

	/*
	TEST_METHOD(debug_mapping_qc_runtime)
	{
		//chrX
		EXECUTE("MappingQC", "-in C:\\Marc\\NA12878_17.bam  -roi C:\\Marc\\nxLRRK2_SNCA_2016_10_25.bed -out C:\\Marc\\test_chrx_out1_bam.qcML  -debug -read_qc C:\\Marc\\test_chrx_out2_bam.qcML");
		foreach(QString line, Helper::loadTextFile(lastLogFile(), true))
		{
			if (line.contains("took:")) qDebug() << QString("chrX BAM: ")+line;
		}
		EXECUTE("MappingQC", "-in C:\\Marc\\NA12878_17.cram -roi C:\\Marc\\nxLRRK2_SNCA_2016_10_25.bed -out C:\\Marc\\test_chrx_out1_cram.qcML -debug -read_qc C:\\Marc\\test_chrx_out2_cram.qcML");
		foreach(QString line, Helper::loadTextFile(lastLogFile(), true))
		{
			if (line.contains("took:")) qDebug() << QString("chrX CRAM: ")+line;
		}
		qDebug() << "";

		//WES 100x
		EXECUTE("MappingQC", "-in C:\\Marc\\NA12878x3_13.bam  -roi C:\\Marc\\twistCustomExomeV2_2021_12_14.bed -out C:\\Marc\\test_wes_out1_bam.qcML  -debug -read_qc C:\\Marc\\test_wes_out2_bam.qcML");
		foreach(QString line, Helper::loadTextFile(lastLogFile(), true))
		{
			if (line.contains("took:")) qDebug() << QString("WES BAM: ")+line;
		}
		EXECUTE("MappingQC", "-in C:\\Marc\\NA12878x3_13.cram -roi C:\\Marc\\twistCustomExomeV2_2021_12_14.bed -out C:\\Marc\\test_wes_out1_cram.qcML -debug -read_qc C:\\Marc\\test_wes_out2_cram.qcML");
		foreach(QString line, Helper::loadTextFile(lastLogFile(), true))
		{
			if (line.contains("took:")) qDebug() << QString("WES CRAM: ")+line;
		}
		qDebug() << "";

		//WGS 30x
		EXECUTE("MappingQC", "-in C:\\Marc\\NA12878_45.bam  -wgs -out C:\\Marc\\test_wgs_out1_bam.qcML  -debug -read_qc C:\\Marc\\test_wgs_out2_bam.qcML");
		foreach(QString line, Helper::loadTextFile(lastLogFile(), true))
		{
			if (line.contains("took:")) qDebug() << QString("WGS BAM: ")+line;
		}
		EXECUTE("MappingQC", "-in C:\\Marc\\NA12878_45.cram -wgs -out C:\\Marc\\test_wgs_out1_cram.qcML -debug -read_qc C:\\Marc\\test_wgs_out2_cram.qcML");
		foreach(QString line, Helper::loadTextFile(lastLogFile(), true))
		{
			if (line.contains("took:")) qDebug() << QString("WGS CRAM: ")+line;
		}
	}
	*/

};
