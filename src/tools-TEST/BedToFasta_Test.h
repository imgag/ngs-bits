#include "../TestFramework.h"
#include "Settings.h"

class BedToFasta_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");
	
		TFW_EXEC("BedToFasta", "-in " + QFINDTESTDATA("data_in/BedToFasta_in1.bed") + " -out out/BedToFasta_test01_out.bed -ref " + ref_file);
		TFW::comareFiles("out/BedToFasta_test01_out.bed", QFINDTESTDATA("data_out/BedToFasta_out1.fa"));
	}

};

TFW_DECLARE(BedToFasta_Test)

