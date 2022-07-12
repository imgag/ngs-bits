#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedpeAnnotateFromBed_Test)
{
Q_OBJECT
private slots:

	void std_params()
	{
		//test
		EXECUTE("BedpeAnnotateFromBed", "-in " + TESTDATA("data_in/BedpeAnnotateFromBed_in1.bedpe") + " -bed " + TESTDATA("data_in/BedpeAnnotateFromBed_bed.bed")
				+ " -out out/BedpeAnnotateFromBed_out1.bedpe -url_decode -col_name OMIM");

		COMPARE_FILES("out/BedpeAnnotateFromBed_out1.bedpe", TESTDATA("data_out/BedpeAnnotateFromBed_out1.bedpe"));
	}

	void replace_underscores()
	{
		//test
		EXECUTE("BedpeAnnotateFromBed", "-in " + TESTDATA("data_in/BedpeAnnotateFromBed_in1.bedpe") + " -bed " + TESTDATA("data_in/BedpeAnnotateFromBed_bed.bed")
				+ " -out out/BedpeAnnotateFromBed_out2.bedpe -url_decode -col_name OMIM -replace_underscore");

		COMPARE_FILES("out/BedpeAnnotateFromBed_out2.bedpe", TESTDATA("data_out/BedpeAnnotateFromBed_out2.bedpe"));
	}

	void reannotate()
	{
		//test
		EXECUTE("BedpeAnnotateFromBed", "-in " + TESTDATA("data_in/BedpeAnnotateFromBed_in2.bedpe") + " -bed " + TESTDATA("data_in/BedpeAnnotateFromBed_bed.bed")
				+ " -out out/BedpeAnnotateFromBed_out2.bedpe -url_decode -col_name OMIM -replace_underscore");

		COMPARE_FILES("out/BedpeAnnotateFromBed_out2.bedpe", TESTDATA("data_out/BedpeAnnotateFromBed_out2.bedpe"));
	}

	void list()
	{
		//test
		EXECUTE("BedpeAnnotateFromBed", "-in " + TESTDATA("data_in/BedpeAnnotateFromBed_in3.bedpe") + " -bed " + TESTDATA("data_in/BedpeAnnotateFromBed_bed_int.bed")
				+ " -out out/BedpeAnnotateFromBed_out3.bedpe -col_name LIST -no_duplicates");

		COMPARE_FILES("out/BedpeAnnotateFromBed_out3.bedpe", TESTDATA("data_out/BedpeAnnotateFromBed_out3.bedpe"));
	}

	void max_int()
	{
		//test
		EXECUTE("BedpeAnnotateFromBed", "-in " + TESTDATA("data_in/BedpeAnnotateFromBed_in3.bedpe") + " -bed " + TESTDATA("data_in/BedpeAnnotateFromBed_bed_int.bed")
				+ " -out out/BedpeAnnotateFromBed_out4.bedpe -max_value -col_name INTEGER");

		COMPARE_FILES("out/BedpeAnnotateFromBed_out4.bedpe", TESTDATA("data_out/BedpeAnnotateFromBed_out4.bedpe"));
	}

	void max_double()
	{
		//test
		EXECUTE("BedpeAnnotateFromBed", "-in " + TESTDATA("data_in/BedpeAnnotateFromBed_in3.bedpe") + " -bed " + TESTDATA("data_in/BedpeAnnotateFromBed_bed_double.bed")
				+ " -out out/BedpeAnnotateFromBed_out5.bedpe -max_value -col_name DOUBLE");

		COMPARE_FILES("out/BedpeAnnotateFromBed_out5.bedpe", TESTDATA("data_out/BedpeAnnotateFromBed_out5.bedpe"));
	}

	void only_breakpoints()
	{
		//test
		EXECUTE("BedpeAnnotateFromBed", "-in " + TESTDATA("data_in/BedpeAnnotateFromBed_in3.bedpe") + " -bed " + TESTDATA("data_in/BedpeAnnotateFromBed_bed_int.bed")
				+ " -out out/BedpeAnnotateFromBed_out6.bedpe -max_value -col_name INTEGER -only_breakpoints");

		COMPARE_FILES("out/BedpeAnnotateFromBed_out6.bedpe", TESTDATA("data_out/BedpeAnnotateFromBed_out6.bedpe"));
	}

};


