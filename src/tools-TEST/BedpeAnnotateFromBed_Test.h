#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

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

};


