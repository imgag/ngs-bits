#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateFromBed_Test)
{
Q_OBJECT
private slots:

	void normal_mode_with_existing_annotations()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in1.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -no_duplicates -out out/BedAnnotateFromBed_out1.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out1.bed", TESTDATA("data_out/BedAnnotateFromBed_out1.bed"));
	}

	void normal_mode_without_existing_annotations()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in2.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -col 5 -no_duplicates -out out/BedAnnotateFromBed_out2.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out2.bed", TESTDATA("data_out/BedAnnotateFromBed_out2.bed"));
	}

	void normal_mode_with_existing_annotations_and_clear_and_duplicate()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in2.bed") + " -clear -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -col 5 -out out/BedAnnotateFromBed_out3.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out3.bed", TESTDATA("data_out/BedAnnotateFromBed_out3.bed"));
	}

	void normal_mode_special_handling_of_tsv_header()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in3.tsv") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -no_duplicates -out out/BedAnnotateFromBed_out4.tsv");
		COMPARE_FILES("out/BedAnnotateFromBed_out4.tsv", TESTDATA("data_out/BedAnnotateFromBed_out4.tsv"));
	}

	void overlap_mode()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in3.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -overlap -no_duplicates -out out/BedAnnotateFromBed_out5.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out5.bed", TESTDATA("data_out/BedAnnotateFromBed_out5.bed"));
	}

	void url_decoding()
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in1.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db2.bed") + " -url_decode -no_duplicates -out out/BedAnnotateFromBed_out6.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out6.bed", TESTDATA("data_out/BedAnnotateFromBed_out6.bed"));
	}
};
