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
		VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out1.vcf");
	}

	void test_with_parameters()
	{
		EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out2.vcf -source " + TESTDATA("data_in/VcfAnnotateFromVcf_an2_NGSD.vcf.gz") + " -info_keys COUNTS,GSC01=GROUP,HAF,CLAS,CLAS_COM,COM -id_column ID -prefix NGSD" );
		COMPARE_FILES("out/VcfAnnotateFromVcf_out2.vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out2.vcf"));
		VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out2.vcf");
	}

	//multi-thread test
	void test_multithread()
	{
		for (int i=2; i<=8; ++i)
		{
			QString suffix = QString::number(i) + "threads";

			EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out1_" + suffix +".vcf -config_file " + TESTDATA("data_in/VcfAnnotateFromVcf_config.tsv") + " -block_size 30 -threads " + QString::number(i));
			COMPARE_FILES("out/VcfAnnotateFromVcf_out1_" + suffix +".vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out1.vcf"));
			VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out1_" + suffix +".vcf");

			EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out2_" + suffix +".vcf -source " + TESTDATA("data_in/VcfAnnotateFromVcf_an2_NGSD.vcf.gz") + " -info_keys COUNTS,GSC01=GROUP,HAF,CLAS,CLAS_COM,COM -id_column ID -prefix NGSD -block_size 30 -threads " + QString::number(i) );
			COMPARE_FILES("out/VcfAnnotateFromVcf_out2_" + suffix +".vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out2.vcf"));
			VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out2_" + suffix +".vcf");
		}
	}

	void test_with_unordered_info_ids()
	{
		EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out3.vcf -source " + TESTDATA("data_in/VcfAnnotateFromVcf_an2_NGSD.vcf.gz") + " -info_keys GSC01=GROUP,CLAS,COM,CLAS_COM,COUNTS,HAF -id_column ID -prefix NGSD" );
		COMPARE_FILES("out/VcfAnnotateFromVcf_out3.vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out3.vcf"));
		VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out3.vcf");
	}

	void test_with_parameters_id_only()
	{
		EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out4.vcf -source " + TESTDATA("data_in/VcfAnnotateFromVcf_an2_NGSD.vcf.gz") + " -id_column ID -prefix NGSD" );
		COMPARE_FILES("out/VcfAnnotateFromVcf_out4.vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out4.vcf"));
		VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out4.vcf");
	}

	void test_with_config_file_and_existence_only()
	{
		EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -out out/VcfAnnotateFromVcf_out5.vcf -config_file " + TESTDATA("data_in/VcfAnnotateFromVcf_config2.tsv") );
		COMPARE_FILES("out/VcfAnnotateFromVcf_out5.vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out5.vcf"));
		VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out5.vcf");
	}

	void test_with_existence_only_from_cli()
	{
		EXECUTE("VcfAnnotateFromVcf", "-in " + TESTDATA("data_in/VcfAnnotateFromVcf_in1.vcf") + " -source " + TESTDATA("data_in/VcfAnnotateFromVcf_an3_ExOnly.vcf.gz") + " -out out/VcfAnnotateFromVcf_out6.vcf -existence_only");
		COMPARE_FILES("out/VcfAnnotateFromVcf_out6.vcf", TESTDATA("data_out/VcfAnnotateFromVcf_out6.vcf"));
		VCF_IS_VALID_HG19("out/VcfAnnotateFromVcf_out6.vcf");
	}

};
