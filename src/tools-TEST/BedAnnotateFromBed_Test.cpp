#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateFromBed_Test)
{
private:

	TEST_METHOD(normal_mode_with_existing_annotations)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in1.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -no_duplicates -out out/BedAnnotateFromBed_out1.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out1.bed", TESTDATA("data_out/BedAnnotateFromBed_out1.bed"));
	}

	TEST_METHOD(normal_mode_without_existing_annotations)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in2.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -col 5 -no_duplicates -out out/BedAnnotateFromBed_out2.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out2.bed", TESTDATA("data_out/BedAnnotateFromBed_out2.bed"));
	}

	TEST_METHOD(normal_mode_with_existing_annotations_and_clear_and_duplicate)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in2.bed") + " -clear -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -col 5 -out out/BedAnnotateFromBed_out3.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out3.bed", TESTDATA("data_out/BedAnnotateFromBed_out3.bed"));
	}

	TEST_METHOD(normal_mode_special_handling_of_tsv_header)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in3.tsv") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -no_duplicates -name tsv_header -out out/BedAnnotateFromBed_out4.tsv");
		COMPARE_FILES("out/BedAnnotateFromBed_out4.tsv", TESTDATA("data_out/BedAnnotateFromBed_out4.tsv"));
	}

	TEST_METHOD(overlap_mode)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in3.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -overlap -no_duplicates -out out/BedAnnotateFromBed_out5.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out5.bed", TESTDATA("data_out/BedAnnotateFromBed_out5.bed"));
	}

	TEST_METHOD(url_decoding)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in1.bed") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db2.bed") + " -url_decode -no_duplicates -out out/BedAnnotateFromBed_out6.bed");
		COMPARE_FILES("out/BedAnnotateFromBed_out6.bed", TESTDATA("data_out/BedAnnotateFromBed_out6.bed"));
	}

	TEST_METHOD(reannotate_tsv_file)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_out/BedAnnotateFromBed_out4.tsv") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -no_duplicates -name tsv_header -out out/BedAnnotateFromBed_out7.tsv");
		COMPARE_FILES("out/BedAnnotateFromBed_out7.tsv", TESTDATA("data_out/BedAnnotateFromBed_out4.tsv"));
	}

	TEST_METHOD(reannotate_tsv_file_overlap)
	{
		EXECUTE("BedAnnotateFromBed", "-in " + TESTDATA("data_in/BedAnnotateFromBed_in8.tsv") + " -in2 " + TESTDATA("data_in/BedAnnotateFromBed_db1.bed") + " -overlap -no_duplicates -name tsv_header -out out/BedAnnotateFromBed_out8.tsv");
		COMPARE_FILES("out/BedAnnotateFromBed_out8.tsv", TESTDATA("data_out/BedAnnotateFromBed_out8.tsv"));
	}
};
