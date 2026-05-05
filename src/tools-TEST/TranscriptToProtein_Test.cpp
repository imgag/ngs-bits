#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(TranscriptToProtein_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/TranscriptToProtein_in.sql"));

		EXECUTE("TranscriptToProtein", "-in " + TESTDATA("data_in/TranscriptToProtein_in.txt") + " -out out/TranscriptToProtein_out.txt");
		COMPARE_FILES("out/TranscriptToProtein_out.txt", TESTDATA("data_out/TranscriptToProtein_out.txt"));
	}
	
};
