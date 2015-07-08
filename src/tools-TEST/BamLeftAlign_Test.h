#include "TestFramework.h"
#include "Settings.h"

class BamLeftAlign_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		TFW_EXEC("BamLeftAlign", "-in " + QFINDTESTDATA("data_in/BamLeftAlign_in1.bam") + " -out out/BamLeftAlign_out1.bam -ref " + ref_file + " -v");
		QVERIFY(QFile::exists("out/BamLeftAlign_out1.bam"));
		TFW::comareFiles("out/BamLeftAlign_Test_line16.log", QFINDTESTDATA("data_out/BamLeftAlign_out1.log"));
	}
	
};

TFW_DECLARE(BamLeftAlign_Test)
