#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfMultiBreak_Test)
{
Q_OBJECT
private slots:
    void test_multi_allele() {
        EXECUTE("VcfMultibreak", "-in " + TESTDATA("data_in/VcfMultibreak_in1.vcf") + " -out out/VcfMultibreak_out1.vcf");
        COMPARE_FILES("out/VcfMultiBreak_out1.vcf", TESTDATA("data_out/VcfMultiBreak_out1.vcf"));
    }

    void test_no_allele() {
        // This should leave everything unchanged
        EXECUTE("VcfMultibreak", "-in " + TESTDATA("data_in/VcfMultibreak_in2.vcf") + " -out out/VcfMultibreak_out2.vcf");
        COMPARE_FILES("out/VcfMultibreak_out2.vcf", TESTDATA("data_out/VcfMultibreak_out2.vcf"));
    }
};
