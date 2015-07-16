#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BamLeftAlign_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("BamLeftAlign", "-in " + TESTDATA("data_in/BamLeftAlign_in1.bam") + " -out out/BamLeftAlign_out1.bam -ref " + ref_file + " -v");
		IS_TRUE(QFile::exists("out/BamLeftAlign_out1.bam"));
		COMPARE_FILES("out/BamLeftAlign_Test_line14.log", TESTDATA("data_out/BamLeftAlign_out1.log"));
	}
	
};
