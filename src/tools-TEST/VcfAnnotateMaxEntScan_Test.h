#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(VcfAnnotateMaxEntScan_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfAnnotateMaxEntScan", "-in " + TESTDATA("data_in/VcfAnnotateMaxEntScan.vcf") + " -out out/VcfAnnotateMaxEntScan_out.vcf" + " -gff " + TESTDATA("data_in/VcfAnnotateMaxEntScan_transcripts.gff3") + "-ref" + "https://download.imgag.de/public/genomes/GRCh38.fa" + " -mes" + " -swa");
		COMPARE_FILES("out/VcfAnnotateMaxEntScan_out.vcf", TESTDATA("data_out/VcfAnnotateMaxEntScan_out.vcf"));
		VCF_IS_VALID("out/VcfAnnotateMaxEntScan_out.vcf");
	}

};
