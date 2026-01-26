#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(SamplePath_Test)
{
private:
	
	TEST_METHOD(type_sample_folder)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_01");

		QString path = Helper::loadTextFile(lastLogFile(), true).join("").trimmed();
		while (path.endsWith("/") || path.endsWith("\\")) path.chop(1);
		IS_TRUE(path.endsWith("Sample_NA12878_01"));
	}

	TEST_METHOD(type_bam)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_01 -type BAM");

		QString path = Helper::loadTextFile(lastLogFile(), true).join("").trimmed();
		IS_TRUE(path.endsWith("NA12878_01.bam"));
	}

	TEST_METHOD(project_override)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_02");

		QString path = Helper::loadTextFile(lastLogFile(), true).join("").trimmed();
		IS_TRUE(path.contains("or_project"));
	}

	TEST_METHOD(sample_override)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_03");

		QString path = Helper::loadTextFile(lastLogFile(), true).join("").trimmed();
		IS_TRUE(path.contains("or_sample"));
	}
};


