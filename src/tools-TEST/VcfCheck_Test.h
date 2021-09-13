#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfCheck_Test)
{
Q_OBJECT
private slots:
	
	void no_warnings()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in1.vcf") + " -out out/VcfCheck_out1.txt");
		COMPARE_FILES("out/VcfCheck_out1.txt", TESTDATA("data_out/VcfCheck_out1.txt"));
	}

	void no_warnings_with_info_lines()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in1.vcf") + " -out out/VcfCheck_out2.txt -info -lines 200");
		COMPARE_FILES("out/VcfCheck_out2.txt", TESTDATA("data_out/VcfCheck_out2.txt"));
	}

	void with_warnings()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in2.vcf") + " -out out/VcfCheck_out3.txt");
		REMOVE_LINES("out/VcfCheck_out3.txt", QRegExp("^chr"));
		COMPARE_FILES("out/VcfCheck_out3.txt", TESTDATA("data_out/VcfCheck_out3.txt"));
	}

	void no_warnings_vcfgz()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in1.vcf.gz") + " -out out/VcfCheck_out4.txt");
		COMPARE_FILES("out/VcfCheck_out4.txt", TESTDATA("data_out/VcfCheck_out1.txt"));
	}

	void empty_info_column()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE_FAIL("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in3.vcf") + " -out out/VcfCheck_out5.txt");
		REMOVE_LINES("out/VcfCheck_out5.txt", QRegExp("^chr"));
		COMPARE_FILES("out/VcfCheck_out5.txt", TESTDATA("data_out/VcfCheck_out5.txt"));
	}
};

