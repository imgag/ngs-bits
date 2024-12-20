#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(RohHunter_Test)
{
Q_OBJECT
private slots:

	void default_values()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in1.vcf.gz") + " -out out/RohHunter_out1.tsv -var_af_keys_vep gnomAD_AF,AF");
        COMPARE_FILES("out/RohHunter_out1.tsv", TESTDATA("data_out/RohHunter_out1.tsv"));
	}

	void with_chrX()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in1.vcf.gz") + " -out out/RohHunter_out2.tsv -var_af_keys_vep gnomAD_AF,AF -inc_chrx");
		COMPARE_FILES("out/RohHunter_out2.tsv", TESTDATA("data_out/RohHunter_out2.tsv"));
	}

	void with_annotate()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in1.vcf.gz") + " -out out/RohHunter_out3.tsv -var_af_keys_vep gnomAD_AF,AF -annotate " + TESTDATA("data_in/RohHunter_genes.bed"));
		COMPARE_FILES("out/RohHunter_out3.tsv", TESTDATA("data_out/RohHunter_out3.tsv"));
	}

	//Note: 'RohHunter_in2.vcf' contains the same variants as 'RohHunter_in1.vcf', but with annotations in INFO fiels > the same ROHs should come out
	void annotations_in_info_field_instead_of_vep()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in2.vcf.gz") + " -out out/RohHunter_out4.tsv -var_af_keys gnomAD_AF,AF");
		COMPARE_FILES("out/RohHunter_out4.tsv", TESTDATA("data_out/RohHunter_out1.tsv"));
	}

};
