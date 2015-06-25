#include "../TestFramework.h"

class BamClipOverlap_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("BamClipOverlap", "-in " + QFINDTESTDATA("data_in/BamClipOverlap_in1.bam") + " -out out/BamClipOverlap_out1.bam -v");
		QVERIFY(QFile::exists("out/BamClipOverlap_out1.bam"));
        TFW::comareFiles("out/BamClipOverlap_Test_line12.log", QFINDTESTDATA("data_out/BamClipOverlap_out1.log"));
	}
	
};

TFW_DECLARE(BamClipOverlap_Test)
