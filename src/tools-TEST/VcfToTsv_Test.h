#include "TestFramework.h"

TEST_CLASS(VcfToTsv_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		EXECUTE("VcfToTsv", "-in " + TESTDATA("data_in/VcfToTsv_in1.vcf") + " -out out/VcfToTsv_out1.tsv");
		COMPARE_FILES("out/VcfToTsv_out1.tsv", TESTDATA("data_out/VcfToTsv_out1.tsv"));
	}
};

