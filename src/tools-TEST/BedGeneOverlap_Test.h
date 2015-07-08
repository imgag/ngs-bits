#include "TestFramework.h"
#include "Settings.h"

class BedGeneOverlap_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		QString db_file = Settings::string("ccds_merged");
		if (db_file=="") QSKIP("Test needs a database file!");

		TFW_EXEC("BedGeneOverlap", "-in " + QFINDTESTDATA("data_in/BedGeneOverlap_in1.bed") + " -out out/BedGeneOverlap_out1.tsv -db " + db_file);
		TFW::comareFiles("out/BedGeneOverlap_out1.tsv", QFINDTESTDATA("data_out/BedGeneOverlap_out1.tsv"));
	}

};

TFW_DECLARE(BedGeneOverlap_Test)


