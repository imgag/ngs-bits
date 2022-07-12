#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VariantAnnotateASE_Test)
{
Q_OBJECT
private slots:

	void germline()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VariantAnnotateASE", "-in " + TESTDATA("data_in/VariantAnnotateASE_in1.GSvar") + " -bam " + TESTDATA("data_in/rnaseq.bam") + " -out out/VariantAnnotateASE_out1.GSvar -ref " + ref_file);
		REMOVE_LINES("out/VariantAnnotateASE_out1.GSvar", QRegExp("^##VariantAnnotateASE_BAM="));
		COMPARE_FILES("out/VariantAnnotateASE_out1.GSvar", TESTDATA("data_out/VariantAnnotateASE_out1.GSvar"));
	}

	void somatic()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		EXECUTE("VariantAnnotateASE", "-in " + TESTDATA("data_in/VariantAnnotateASE_in2.GSvar") + " -bam " + TESTDATA("data_in/rnaseq.bam") + " -out out/VariantAnnotateASE_out2.GSvar -ref " + ref_file);
		REMOVE_LINES("out/VariantAnnotateASE_out2.GSvar", QRegExp("^##VariantAnnotateASE_BAM="));
		COMPARE_FILES("out/VariantAnnotateASE_out2.GSvar", TESTDATA("data_out/VariantAnnotateASE_out2.GSvar"));
	}

};
