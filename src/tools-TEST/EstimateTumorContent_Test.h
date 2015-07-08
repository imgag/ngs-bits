#include "TestFramework.h"

class EstimateTumorContent_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		QString tu = "/mnt/share/data/test_data/GS120240-GS120180_im_filtered.tsv";
		QString tu_bam = "/mnt/share/data/test_data/GS120240.bam";
		QString no_bam = "/mnt/share/data/test_data/GS120180.bam";
	
#ifdef WIN32
		if (!QDir("W:\\share\\").exists()) QSKIP("Test needs data from W: drive!");
	
		tu = "W:\\share\\data\\test_data\\GS120240-GS120180_im_filtered.tsv";
		tu_bam = "W:\\share\\data\\test_data\\GS120240.bam";
		no_bam = "W:\\share\\data\\test_data\\GS120180.bam";
#endif
	
		TFW_EXEC("EstimateTumorContent", "-tu " + tu + " -tu_bam " + tu_bam + " -no_bam " + no_bam + " -out out/EstimateTumorContent_out1.txt");
		TFW::comareFiles("out/EstimateTumorContent_out1.txt", QFINDTESTDATA("data_out/EstimateTumorContent_out1.txt"));
	}

};

TFW_DECLARE(EstimateTumorContent_Test)


