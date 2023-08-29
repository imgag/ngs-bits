#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(SamplePath_Test)
{
Q_OBJECT
private slots:
	
	void type_sample_folder()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_01");

		QString path = Helper::loadTextFile("out/SamplePath_Test_line20.log", true).join("").trimmed();
		while (path.endsWith("/") || path.endsWith("\\")) path.chop(1);
		IS_TRUE(path.endsWith("Sample_NA12878_01"));
	}

	void type_bam()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_01 -type BAM");

		QString path = Helper::loadTextFile("out/SamplePath_Test_line37.log", true).join("").trimmed();
		IS_TRUE(path.endsWith("NA12878_01.bam"));
	}

	void project_override()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_02");

		QString path = Helper::loadTextFile("out/SamplePath_Test_line53.log", true).join("").trimmed();
		IS_TRUE(path.contains("or_project"));
	}

	void sample_override()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SamplePath_init.sql"));

		//test
		EXECUTE("SamplePath", "-test -ps NA12878_03");

		QString path = Helper::loadTextFile("out/SamplePath_Test_line69.log", true).join("").trimmed();
		IS_TRUE(path.contains("or_sample"));
	}
};


