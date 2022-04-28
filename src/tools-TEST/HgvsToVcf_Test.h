#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(HgvsToVcf_Test)
{
Q_OBJECT
private slots:
	
	void test1()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("HgvsToVcf", "-in " + TESTDATA("/data_in/HgvsToVcf_in1.tsv") + " -out out/HgvsToVcf_out1.vcf" + " -ref " + ref_file);
		REMOVE_LINES("out/HgvsToVcf_out1.vcf", QRegExp("##fileDate="));
		COMPARE_FILES("out/HgvsToVcf_out1.vcf", TESTDATA("data_out/HgvsToVcf_out1.vcf"));
		VCF_IS_VALID("out/HgvsToVcf_out1.vcf");
	}


};


