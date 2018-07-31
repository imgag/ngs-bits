#include "TestFramework.h"

TEST_CLASS(VariantFilterAnnotations_Test)
{
Q_OBJECT
private slots:
	
	void no_filter()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters1.txt") + " -out out/VariantFilterAnnotations_out1.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out1.tsv", TESTDATA("data_out/VariantFilterAnnotations_out1.tsv"));
	}

	void maxAf()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters2.txt") + " -out out/VariantFilterAnnotations_out2.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out2.tsv", TESTDATA("data_out/VariantFilterAnnotations_out2.tsv"));
	}

	void impact_ihdb()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters3.txt") + " -out out/VariantFilterAnnotations_out3.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out3.tsv", TESTDATA("data_out/VariantFilterAnnotations_out3.tsv"));
	}

	void class_filters_genoAffected()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters4.txt") + " -out out/VariantFilterAnnotations_out4.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out4.tsv", TESTDATA("data_out/VariantFilterAnnotations_out4.tsv"));
	}

	void maxSubAf()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters5.txt") + " -out out/VariantFilterAnnotations_out5.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out5.tsv", TESTDATA("data_out/VariantFilterAnnotations_out5.tsv"));
	}

	void multisample_maxAf_dominant()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters6.txt") + " -out out/VariantFilterAnnotations_out6.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out6.tsv", TESTDATA("data_out/VariantFilterAnnotations_out6.tsv"));
	}

	void multisample_maxAf_recessive()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters7.txt") + " -out out/VariantFilterAnnotations_out7.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out7.tsv", TESTDATA("data_out/VariantFilterAnnotations_out7.tsv"));
	}

	void multisample_notWT()
	{
		EXECUTE("VariantFilterAnnotations", "-in " + TESTDATA("data_in/VariantFilterAnnotations_in_multi.GSvar") + " -filters " + TESTDATA("data_in/VariantFilterAnnotations_filters8.txt") + " -out out/VariantFilterAnnotations_out8.tsv");
		COMPARE_FILES("out/VariantFilterAnnotations_out8.tsv", TESTDATA("data_out/VariantFilterAnnotations_out8.tsv"));
	}

};


