#include "TestFramework.h"

TEST_CLASS(Cidx_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QFile::copy(TESTDATA("../cppNGS-TEST/data_in/ChromosomalFileIndex.tsv"), "out/ChromosomalFileIndex.tsv");
		QFile::remove("out/ChromosomalFileIndex.tsv.cidx");
		EXECUTE("Cidx", "-in out/ChromosomalFileIndex.tsv -out out/Cidx_test01_out.tsv -pos chr1:866505-866518 -b 10");
		COMPARE_FILES("out/Cidx_test01_out.tsv", TESTDATA("data_out/Cidx_test01_out.tsv"));
	}
	
	void test_02()
	{
		QFile::copy(TESTDATA("../cppNGS-TEST/data_in/ChromosomalFileIndex2.gff"), "out/ChromosomalFileIndex2.gff");
		QFile::remove("out/ChromosomalFileIndex2.gff.cidx");
		EXECUTE("Cidx", "-in out/ChromosomalFileIndex2.gff -out out/Cidx_test02_out.tsv -pos 1:0-1000000 -b 1000 -c 0 -s 3 -e 4 -h #");
		COMPARE_FILES("out/Cidx_test02_out.tsv", TESTDATA("data_out/Cidx_test02_out.tsv"));
	}

};
