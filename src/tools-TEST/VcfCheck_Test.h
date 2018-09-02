#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfCheck_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in1.vcf") + " -out out/VcfCheck_out1.txt");
		COMPARE_FILES("out/VcfCheck_out1.txt", TESTDATA("data_out/VcfCheck_out1.txt"));
	}

};

