#include "TestFramework.h"

TEST_CLASS(TranscriptComparison_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		EXECUTE("TranscriptComparison", "-ensembl " + TESTDATA("data_in/TranscriptComparison_ensembl.gff3") + " -refseq " + TESTDATA("data_in/TranscriptComparison_refseq.gff3") + " -out out/TranscriptComparison_out1.tsv");
		REMOVE_LINES("out/TranscriptComparison_out1.tsv", QRegExp("##.*file:"));
		COMPARE_FILES("out/TranscriptComparison_out1.tsv", TESTDATA("data_out/TranscriptComparison_out1.tsv"));
	}

};
