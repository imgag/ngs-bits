#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportGenes_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGenes_init.sql"));

		//test
		EXECUTE("NGSDExportGenes", "-test -out out/NGSDExportGenes_out1.tsv");
		COMPARE_FILES("out/NGSDExportGenes_out1.tsv", TESTDATA("data_out/NGSDExportGenes_out1.tsv"));
	}

	void with_disease_info()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportGenes_init.sql"));

		//test
		EXECUTE("NGSDExportGenes", "-test -add_disease_info -out out/NGSDExportGenes_out2.tsv");
		COMPARE_FILES("out/NGSDExportGenes_out2.tsv", TESTDATA("data_out/NGSDExportGenes_out2.tsv"));
	}
};

