#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(GenesToApproved_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString db_file = Settings::string("hgnc");
		if (db_file=="") SKIP("Test needs a database file!");

		EXECUTE("GenesToApproved", "-in " + TESTDATA("data_in/GenesToApproved_in1.txt") + " -out out/GenesToApproved_out1.txt -db " + db_file);
		COMPARE_FILES("out/GenesToApproved_out1.txt", TESTDATA("data_out/GenesToApproved_out1.txt"));
	}

};

