#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportGff_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init db
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGff_init.sql"));

		//test
		EXECUTE("NGSDExportGff", "-test -out out/NGSDExportGff_out1.gff3");
		COMPARE_FILES("out/NGSDExportGff_out1.gff3", TESTDATA("data_out/NGSDExportGff_out1.gff3"));
	}

	void with_genes()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init db
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGff_init.sql"));

		//test
		EXECUTE("NGSDExportGff", "-test -genes -out out/NGSDExportGff_out2.gff3");
		COMPARE_FILES("out/NGSDExportGff_out2.gff3", TESTDATA("data_out/NGSDExportGff_out2.gff3"));
	}
};

