#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateFromBed_Test)
{
Q_OBJECT
private slots:

	void with_existing_annotations()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in1.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -out out/BedAnnotateFromBed_out1.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out1.bed", TESTDATA("data_out/BedAnnotateFromBed_out1.bed"));
	}

	void without_existing_annotations()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in2.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db2.bed") + " -out out/BedAnnotateFromBed_out2.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out2.bed", TESTDATA("data_out/BedAnnotateFromBed_out2.bed"));
	}

	void with_existing_annotations_and_clear()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in1.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -out out/BedAnnotateFromBed_out3.bed -clear");
		COMPARE_FILES("out/BedAnnotateFromBed_out3.bed", TESTDATA("data_out/BedAnnotateFromBed_out3.bed"));
	}

	void special_handling_of_tsv_header()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in3.tsv") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -out out/BedAnnotateFromBed_out4.tsv");
		COMPARE_FILES("out/BedAnnotateFromBed_out4.tsv", TESTDATA("data_out/BedAnnotateFromBed_out4.tsv"));
	}
};
