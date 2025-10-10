#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDInit_Test)
{
private:
	
	TEST_METHOD(import_NA12878_03)
	{
		SKIP_IF_NO_TEST_NGSD();

		//test
		EXECUTE("NGSDInit", "-test -add " + TESTDATA("data_in/NGSDInit_in1.sql"));
	}
};


