#include "TestFramework.h"

TEST_CLASS(Cidx_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QFile::copy(QFINDTESTDATA("../cppNGS-TEST/data_in/ChromosomalFileIndex.tsv"), "out/ChromosomalFileIndex.tsv");
		QFile::remove("out/ChromosomalFileIndex.tsv.cidx");
		TFW_EXEC("Cidx", "-in out/ChromosomalFileIndex.tsv -out out/Cidx_test01_out.tsv -pos chr1:866505-866518 -b 10");
		TFW::comareFiles("out/Cidx_test01_out.tsv", QFINDTESTDATA("data_out/Cidx_test01_out.tsv"));
	}
	
	void test_02()
	{
		QFile::copy(QFINDTESTDATA("../cppNGS-TEST/data_in/ChromosomalFileIndex2.gff"), "out/ChromosomalFileIndex2.gff");
		QFile::remove("out/ChromosomalFileIndex2.gff.cidx");
		TFW_EXEC("Cidx", "-in out/ChromosomalFileIndex2.gff -out out/Cidx_test02_out.tsv -pos 1:0-1000000 -b 1000 -c 0 -s 3 -e 4 -h #");
		TFW::comareFiles("out/Cidx_test02_out.tsv", QFINDTESTDATA("data_out/Cidx_test02_out.tsv"));
	}

};
