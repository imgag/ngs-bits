#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfLeftAlign_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfLeftAlign", "-in " + TESTDATA("data_in/VcfLeftAlign_in1.vcf") + " -out out/VcfLeftAlign_out1.vcf -ref " + ref_file);
		COMPARE_FILES("out/VcfLeftAlign_out1.vcf", TESTDATA("data_out/VcfLeftAlign_out1.vcf"));
	}
	

	void test_02()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfLeftAlign", "-in " + TESTDATA("data_in/VcfLeftAlign_in2.vcf") + " -out out/VcfLeftAlign_out2.vcf -ref " + ref_file);
		COMPARE_FILES("out/VcfLeftAlign_out2.vcf", TESTDATA("data_out/VcfLeftAlign_out2.vcf"));
	}

};

