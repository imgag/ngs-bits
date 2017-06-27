#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(SomaticQC_Test)
{
Q_OBJECT
private slots:

    void no_target()
    {
        EXECUTE("SomaticQC", "-tumor_bam " + TESTDATA("../cppNGS-TEST/data_in/tumor.bam") + " -normal_bam " + TESTDATA("../cppNGS-TEST/data_in/normal.bam") + " -somatic_vcf " + TESTDATA("../cppNGS-TEST/data_in/somatic.vcf") + " -links " + TESTDATA("data_in/SomaticQC_in4.qcML") + " -skip_plots -out out/SomaticQC_out1.qcML");
        REMOVE_LINES("out/SomaticQC_out1.qcML", QRegExp("creation "));
        REMOVE_LINES("out/SomaticQC_out1.qcML", QRegExp("<binary>"));
        COMPARE_FILES("out/SomaticQC_out1.qcML", TESTDATA("data_out/SomaticQC_out1.qcML"));
    }
    void exac()
    {
        EXECUTE("SomaticQC", "-tumor_bam " + TESTDATA("../cppNGS-TEST/data_in/tumor.bam") + " -normal_bam " + TESTDATA("../cppNGS-TEST/data_in/normal.bam") + " -somatic_vcf " + TESTDATA("data_in/SomaticQC_in6.vcf") + " -links " + TESTDATA("data_in/SomaticQC_in4.qcML") + " -skip_plots -out out/SomaticQC_out2.qcML");
        REMOVE_LINES("out/SomaticQC_out2.qcML", QRegExp("creation "));
        REMOVE_LINES("out/SomaticQC_out2.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/SomaticQC_out2.qcML", TESTDATA("data_out/SomaticQC_out2.qcML"));
    }
    void no_exac()
    {
        EXECUTE("SomaticQC", "-tumor_bam " + TESTDATA("../cppNGS-TEST/data_in/tumor.bam") + " -normal_bam " + TESTDATA("../cppNGS-TEST/data_in/normal.bam") + " -somatic_vcf " + TESTDATA("data_in/SomaticQC_in7.vcf") + " -links " + TESTDATA("data_in/SomaticQC_in4.qcML") + " -skip_plots -out out/SomaticQC_out3.qcML");
        REMOVE_LINES("out/SomaticQC_out3.qcML", QRegExp("creation "));
        REMOVE_LINES("out/SomaticQC_out3.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/SomaticQC_out3.qcML", TESTDATA("data_out/SomaticQC_out3.qcML"));
    }
    void target()
	{
        QString ref_file = Settings::string("reference_genome");
        if (ref_file=="") SKIP("Test needs the reference genome!");

        EXECUTE("SomaticQC", "-tumor_bam " + TESTDATA("../cppNGS-TEST/data_in/tumor.bam") + " -normal_bam " + TESTDATA("../cppNGS-TEST/data_in/normal.bam") + " -somatic_vcf " + TESTDATA("../cppNGS-TEST/data_in/somatic.vcf") + " -links " + TESTDATA("data_in/SomaticQC_in4.qcML") + " -target_bed " + TESTDATA("data_in/SomaticQC_in5.bed") + " -out out/SomaticQC_out2.qcML");
		REMOVE_LINES("out/SomaticQC_out2.qcML", QRegExp("creation "));
		REMOVE_LINES("out/SomaticQC_out2.qcML", QRegExp("<binary>"));
		COMPARE_FILES("out/SomaticQC_out2.qcML", TESTDATA("data_out/SomaticQC_out2.qcML"));
	}
};
