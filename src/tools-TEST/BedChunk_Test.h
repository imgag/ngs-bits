#include "TestFramework.h"

TEST_CLASS(BedChunk_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		TFW_EXEC("BedChunk", "-in " + QFINDTESTDATA("data_in/BedChunk_in1.bed") + " -out out/BedChunk_out1.bed -n 100");
		TFW::comareFiles("out/BedChunk_out1.bed", QFINDTESTDATA("data_out/BedChunk_out1.bed"));
	}
};
