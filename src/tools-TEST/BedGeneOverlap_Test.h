#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedGeneOverlap_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString db_file = Settings::string("ccds_merged");
		if (db_file=="") SKIP("Test needs a database file!");

		EXECUTE("BedGeneOverlap", "-in " + TESTDATA("data_in/BedGeneOverlap_in1.bed") + " -out out/BedGeneOverlap_out1.tsv -db " + db_file);
		COMPARE_FILES("out/BedGeneOverlap_out1.tsv", TESTDATA("data_out/BedGeneOverlap_out1.tsv"));
	}

};
