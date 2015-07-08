#include "TestFramework.h"

class FastqConvert_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		TFW_EXEC("FastqConvert", "-in " + QFINDTESTDATA("data_in/FastqConvert_in1.fastq.gz") + " -out out/FastqConvert_out1.fastq.gz");
		TFW::comareFilesGZ("out/FastqConvert_out1.fastq.gz", QFINDTESTDATA("data_out/FastqConvert_out1.fastq.gz"));
	}

};

TFW_DECLARE(FastqConvert_Test)

