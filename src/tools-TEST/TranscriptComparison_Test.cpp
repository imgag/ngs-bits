#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(TranscriptComparison_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{

		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/TranscriptComparison_init.sql"));

		EXECUTE("TranscriptComparison", "-ensembl " + TESTDATA("data_in/TranscriptComparison_ensembl.gff3") + " -refseq " + TESTDATA("data_in/TranscriptComparison_refseq.gff3") + " -out out/TranscriptComparison_out1.tsv -test");
        REMOVE_LINES("out/TranscriptComparison_out1.tsv", QRegularExpression("##.*file:"));
		COMPARE_FILES("out/TranscriptComparison_out1.tsv", TESTDATA("data_out/TranscriptComparison_out1.tsv"));
	}

};
