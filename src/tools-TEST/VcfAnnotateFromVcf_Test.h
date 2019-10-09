#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfAnnotateFromVcf_Test)
{
Q_OBJECT
private slots:

	void test_with_config_file()
	{
		EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out1.vcf -config_file " + TESTDATA("data_in/VcfAnnotateFromVcf_config.tsv") );
		COMPARE_FILES("out/VcfAnnotateFromVcf_out1.vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out1.vcf"));
		VCF_IS_VALID("out/VcfAnnotateFromVcf_out1.vcf");
	}

	void test_with_parameters()
	{
		EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out2.vcf -annotation_file " + TESTDATA("data_in/VcfAnnotateFromVcf_an2_NGSD.vcf.gz") + " -info_ids COUNTS,GSC01=GROUP,HAF,CLAS,CLAS_COM,COM -id_column ID -id_prefix NGSD" );
		COMPARE_FILES("out/VcfAnnotateFromVcf_out2.vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out2.vcf"));
		VCF_IS_VALID("out/VcfAnnotateFromVcf_out2.vcf");
	}

};
