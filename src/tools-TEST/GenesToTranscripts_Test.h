#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(GenesToTranscripts_Test)
{
Q_OBJECT
private slots:
	
	void mode_best()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToTranscripts_init.sql"));

		//test
		EXECUTE("GenesToTranscripts", "-test -in " + TESTDATA("data_in/GenesToTranscripts_in1.txt") + " -out out/GenesToTranscripts_out1.tsv -mode best");
		COMPARE_FILES("out/GenesToTranscripts_out1.tsv", TESTDATA("data_out/GenesToTranscripts_out1.tsv"));
	}

	void mode_best_with_version()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToTranscripts_init.sql"));

		//test
		EXECUTE("GenesToTranscripts", "-test -in " + TESTDATA("data_in/GenesToTranscripts_in1.txt") + " -version -out out/GenesToTranscripts_out2.tsv -mode best");
		COMPARE_FILES("out/GenesToTranscripts_out2.tsv", TESTDATA("data_out/GenesToTranscripts_out2.tsv"));
	}

	void mode_relevant()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToTranscripts_init.sql"));

		//test
		EXECUTE("GenesToTranscripts", "-test -in " + TESTDATA("data_in/GenesToTranscripts_in1.txt") + " -out out/GenesToTranscripts_out3.tsv -mode relevant");
		COMPARE_FILES("out/GenesToTranscripts_out3.tsv", TESTDATA("data_out/GenesToTranscripts_out3.tsv"));
	}

	void mode_all()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToTranscripts_init.sql"));

		//test
		EXECUTE("GenesToTranscripts", "-test -in " + TESTDATA("data_in/GenesToTranscripts_in1.txt") + " -out out/GenesToTranscripts_out4.tsv -mode all");
		COMPARE_FILES("out/GenesToTranscripts_out4.tsv", TESTDATA("data_out/GenesToTranscripts_out4.tsv"));
	}

	void mode_mane_select()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToTranscripts_init.sql"));

		//test
		EXECUTE("GenesToTranscripts", "-test -in " + TESTDATA("data_in/GenesToTranscripts_in1.txt") + " -out out/GenesToTranscripts_out5.tsv -mode mane_select");
		COMPARE_FILES("out/GenesToTranscripts_out5.tsv", TESTDATA("data_out/GenesToTranscripts_out5.tsv"));
	}
};

