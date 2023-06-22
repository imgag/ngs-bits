#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(TranscriptsToBed_Test)
{
Q_OBJECT
private slots:
	
	void gene_mode()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/TranscriptsToBed_init.sql"));

		//test
		EXECUTE("TranscriptsToBed", "-test -in " + TESTDATA("data_in/TranscriptsToBed_in1.txt") + " -out out/TranscriptsToBed_out1.bed -mode gene");
		COMPARE_FILES("out/TranscriptsToBed_out1.bed", TESTDATA("data_out/TranscriptsToBed_out1.bed"));
	}

	void exon_mode()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/TranscriptsToBed_init.sql"));

		//test
		EXECUTE("TranscriptsToBed", "-test -in " + TESTDATA("data_in/TranscriptsToBed_in1.txt") + " -out out/TranscriptsToBed_out2.bed -mode exon");
		COMPARE_FILES("out/TranscriptsToBed_out2.bed", TESTDATA("data_out/TranscriptsToBed_out2.bed"));
	}

};

