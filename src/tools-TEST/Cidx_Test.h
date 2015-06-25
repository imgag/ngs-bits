#include "../TestFramework.h"

class Cidx_Test
		: public QObject
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
	
	/* test for debugging a linux problem
	void Cidx_Test::test_99()
	{
		QFile::copy(QFINDTESTDATA("W:/share/data/dbs/HGMD/GFF/hgmd_hg19.gff"), "out/hgmd_hg19.gff");
		QFile::remove("out/hgmd_hg19.gff.cidx");
		TFW_EXEC("Cidx", "-in out/hgmd_hg19.gff -out out/Cidx_test03_out.tsv -s 3 -e 4 -pos X:30686236-30686236");
		TFW::comareFiles("out/Cidx_test03_out.tsv", QFINDTESTDATA("data_out/Cidx_test03_out.tsv"));
	}
	*/

};

TFW_DECLARE(Cidx_Test)

