#include "TestFramework.h"
#include "Settings.h"

class BedAnnotateGenes_Test
		: public QObject
{
	Q_OBJECT

private slots:

	//with default parameters, with 3 columns input
	void test_01()
	{
		QString db_file = Settings::string("ccds");
		if (db_file=="") QSKIP("Test needs a database file!");

		TFW_EXEC("BedAnnotateGenes", "-in " + QFINDTESTDATA("data_in/BedAnnotateGenes_in1.bed") + " -out out/BedAnnotateGenes_out1.bed -db " + db_file);
		TFW::comareFiles("out/BedAnnotateGenes_out1.bed", QFINDTESTDATA("data_out/BedAnnotateGenes_out1.bed"));
	}

	//extended by 25 bases, with 5 columns input
	void test_02()
	{
		QString db_file = Settings::string("ccds");
		if (db_file=="") QSKIP("Test needs a database file!");

		TFW_EXEC("BedAnnotateGenes", "-in " + QFINDTESTDATA("data_in/BedAnnotateGenes_in2.bed") + " -out out/BedAnnotateGenes_out2.bed -extend 25 -db " + db_file);
		TFW::comareFiles("out/BedAnnotateGenes_out2.bed", QFINDTESTDATA("data_out/BedAnnotateGenes_out2.bed"));
	}
};

TFW_DECLARE(BedAnnotateGenes_Test)


