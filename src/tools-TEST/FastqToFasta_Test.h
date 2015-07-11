#include "TestFramework.h"

class FastqToFasta_Test
		: public QObject
{
Q_OBJECT

private slots:
	
	void base_test()
	{
		TFW_EXEC("FastqToFasta", "-in " + QFINDTESTDATA("data_in/FastqToFasta_in1.fastq.gz") + " -out out/FastqToFasta_out1.fasta");
		TFW::comareFilesGZ("out/FastqToFasta_out1.fasta", QFINDTESTDATA("data_out/FastqToFasta_out1.fasta"));
	}

};

TFW_DECLARE(FastqToFasta_Test)


