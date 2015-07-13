#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateGC_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");
	
		EXECUTE("BedAnnotateGC", "-in " + TESTDATA("data_in/BedAnnotateGC_in1.bed") + " -out out/BedAnnotateGC_out1.bed -ref " + ref_file);
		COMPARE_FILES("out/BedAnnotateGC_out1.bed", TESTDATA("data_out/BedAnnotateGC_out1.bed"));
	}

};
