#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateGC_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");
	
		TFW_EXEC("BedAnnotateGC", "-in " + QFINDTESTDATA("data_in/BedAnnotateGC_in1.bed") + " -out out/BedAnnotateGC_out1.bed -ref " + ref_file);
		TFW::comareFiles("out/BedAnnotateGC_out1.bed", QFINDTESTDATA("data_out/BedAnnotateGC_out1.bed"));
	}

};
