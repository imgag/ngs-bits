#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDAnnotateSV_Test)
{
private:

	TEST_METHOD(artifical_svs_new_sample)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAnnotateSV_init.sql"));

		EXECUTE("NGSDAnnotateSV", "-test -in " + TESTDATA("data_in/NGSDAnnotateSV_in1.bedpe") + " -out out/NGSDAnnotateSV_out1.bedpe");

		COMPARE_FILES("out/NGSDAnnotateSV_out1.bedpe", TESTDATA("data_out/NGSDAnnotateSV_out1.bedpe"))
	}
};


