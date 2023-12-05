#include "TestFramework.h"


TEST_CLASS(BamExtract_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("BamExtract", "-in " + TESTDATA("../cppNGS-TEST/data_in/panel.bam") + " -ids " + TESTDATA("data_in/BamExtract_ids.txt") + " -out out/BamExtract_out1.bam -out2 out/BamExtract_out2.bam");
		IS_TRUE(QFile::exists("out/BamExtract_out2.bam"));
		COMPARE_FILES("out/BamExtract_out1.bam", TESTDATA("data_out/BamExtract_out1.bam"));
		COMPARE_FILES("out/BamExtract_Test_line11.log", TESTDATA("data_out/BamCleanHaloplex_out1.log"));
	}
	
};

