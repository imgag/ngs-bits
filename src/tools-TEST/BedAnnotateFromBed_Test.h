#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateFromBed_Test)
{
Q_OBJECT
private slots:

	void with_4th_column()
	{
		//test
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in1.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -out out/BedAnnotateFromBed_out1.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out1.bed", TESTDATA("data_out/BedAnnotateFromBed_out1.bed"));
	}

	void without_4th_column()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in2.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db2.bed") + " -out out/BedAnnotateFromBed_out2.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out2.bed", TESTDATA("data_out/BedAnnotateFromBed_out2.bed"));
	}
};
