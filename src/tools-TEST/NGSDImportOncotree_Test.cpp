#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportOncotree_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();

		//test
		EXECUTE("NGSDImportOncotree", "-test -tree " + TESTDATA("data_in/NGSDImportOncotree_in1.json"));

		//check
		int count = db.getValue("SELECT count(*) FROM oncotree_term").toInt();
		I_EQUAL(count, 898)
		count = db.getValue("SELECT count(*) FROM oncotree_parent").toInt();
		I_EQUAL(count, 897)
		count = db.getValue("SELECT count(*) FROM oncotree_obsolete").toInt();
		I_EQUAL(count, 36)

		//check imported version
		QString version = db.getValue("SELECT version FROM db_import_info WHERE name='oncotree'").toString();
		S_EQUAL(version, "NGSDImportOncotree_in1.json");
	}

};

