#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDSampleUsers_Test)
{
private:
	
	TEST_METHOD(same_sample)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSampleUsers_init.sql"));

		//test
		EXECUTE("NGSDSampleUsers", "-in "+TESTDATA("data_in/NGSDSampleUsers_in1.tsv")+" -out out/NGSDSampleUsers_out1.tsv -test");
		COMPARE_FILES("out/NGSDSampleUsers_out1.tsv", TESTDATA("data_out/NGSDSampleUsers_out1.tsv"));
	}
};
