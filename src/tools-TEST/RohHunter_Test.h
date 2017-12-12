#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(RohHunter_Test)
{
Q_OBJECT
private slots:

	void default_values()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in1.vcf.gz") + " -out out/RohHunter_out1.tsv");
        COMPARE_FILES("out/RohHunter_out1.tsv", TESTDATA("data_out/RohHunter_out1.tsv"));
	}

	void with_chrX()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in1.vcf.gz") + " -out out/RohHunter_out2.tsv -inc_chrx");
		COMPARE_FILES("out/RohHunter_out2.tsv", TESTDATA("data_out/RohHunter_out2.tsv"));
	}

	void with_annotate()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in1.vcf.gz") + " -out out/RohHunter_out3.tsv -annotate " + TESTDATA("data_in/RohHunter_genes.bed"));
		COMPARE_FILES("out/RohHunter_out3.tsv", TESTDATA("data_out/RohHunter_out3.tsv"));
	}

};
