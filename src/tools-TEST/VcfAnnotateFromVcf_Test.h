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

	//multi-thread test
	void test_multithread()
	{
		for (int i=2; i<=8; ++i)
		{
			QString suffix = QString::number(i) + "threads";

			EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out1_" + suffix +".vcf -config_file " + TESTDATA("data_in/VcfAnnotateFromVcf_config.tsv") + " -block_size 30 -threads " + QString::number(i));
			COMPARE_FILES("out/VcfAnnotateFromVcf_out1_" + suffix +".vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out1.vcf"));
			VCF_IS_VALID("out/VcfAnnotateFromVcf_out1_" + suffix +".vcf");

			EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out2_" + suffix +".vcf -annotation_file " + TESTDATA("data_in/VcfAnnotateFromVcf_an2_NGSD.vcf.gz") + " -info_ids COUNTS,GSC01=GROUP,HAF,CLAS,CLAS_COM,COM -id_column ID -id_prefix NGSD -block_size 30 -threads " + QString::number(i) );
			COMPARE_FILES("out/VcfAnnotateFromVcf_out2_" + suffix +".vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out2.vcf"));
			VCF_IS_VALID("out/VcfAnnotateFromVcf_out2_" + suffix +".vcf");
		}
	}

};
