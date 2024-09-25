#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(TranscriptComparison_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{

		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/TranscriptComparison_init.sql"));

		EXECUTE("TranscriptComparison", "-ensembl " + TESTDATA("data_in/TranscriptComparison_ensembl.gff3") + " -refseq " + TESTDATA("data_in/TranscriptComparison_refseq.gff3") + " -out out/TranscriptComparison_out1.tsv");
		REMOVE_LINES("out/TranscriptComparison_out1.tsv", QRegExp("##.*file:"));
		COMPARE_FILES("out/TranscriptComparison_out1.tsv", TESTDATA("data_out/TranscriptComparison_out1.tsv"));
	}

};
