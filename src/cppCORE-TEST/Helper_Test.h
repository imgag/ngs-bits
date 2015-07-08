#include "TestFramework.h"
#include "Helper.h"

class Helper_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void randomString()
	{
		QCOMPARE(Helper::randomString(8).count(), 8);
		QCOMPARE(Helper::randomString(3, "AB").count(), 3);
	}

	void loadTextFile()
	{
		QStringList genes;
		genes = Helper::loadTextFile(QFINDTESTDATA("data_in/genes.txt"), true, '#', true);
		QCOMPARE(genes[0], QString("BRAF"));
		QCOMPARE(genes[1], QString("KRAS"));
		QCOMPARE(genes[2], QString("BRCA1"));
		QCOMPARE(genes[3], QString("BRCA2"));
		QCOMPARE(genes.count(), 4);

		genes = Helper::loadTextFile(QFINDTESTDATA("data_in/genes.txt"), false, '#', true);
		QCOMPARE(genes[0], QString("BRAF"));
		QCOMPARE(genes[1], QString("KRAS"));
		QCOMPARE(genes[2], QString(" "));
		QCOMPARE(genes[3], QString("BRCA1"));
		QCOMPARE(genes[4], QString("BRCA2"));
		QCOMPARE(genes.count(), 5);

		genes = Helper::loadTextFile(QFINDTESTDATA("data_in/genes.txt"), true, QChar::Null, false);
		QCOMPARE(genes[0], QString("#header"));
		QCOMPARE(genes[1], QString("BRAF"));
		QCOMPARE(genes[2], QString("KRAS"));
		QCOMPARE(genes[3], QString(""));
		QCOMPARE(genes[4], QString("BRCA1"));
		QCOMPARE(genes[5], QString("BRCA2"));
		QCOMPARE(genes.count(), 6);
	}

	void storeTextFile()
	{
		QStringList genes;
		genes << "#header" << "Gene1" << "Gene2" << "Gene3";
		Helper::storeTextFile("out/Helper_storeTextFile_out.txt", genes);
		TFW::comareFiles("out/Helper_storeTextFile_out.txt", QFINDTESTDATA("data_out/Helper_storeTextFile_out.txt"));
	}

	void levenshtein()
	{
		QCOMPARE(Helper::levenshtein("", ""), 0);
		QCOMPARE(Helper::levenshtein("abc", "abc"), 0);

		QCOMPARE(Helper::levenshtein("abc", "xbc"), 1);
		QCOMPARE(Helper::levenshtein("abc", "axc"), 1);
		QCOMPARE(Helper::levenshtein("abc", "abx"), 1);
		QCOMPARE(Helper::levenshtein("xbc", "abc"), 1);
		QCOMPARE(Helper::levenshtein("axc", "abc"), 1);
		QCOMPARE(Helper::levenshtein("abx", "abc"), 1);

		QCOMPARE(Helper::levenshtein("abc", "a"), 2);
		QCOMPARE(Helper::levenshtein("abc", "b"), 2);
		QCOMPARE(Helper::levenshtein("abc", "c"), 2);
		QCOMPARE(Helper::levenshtein("a", "abc"), 2);
		QCOMPARE(Helper::levenshtein("b", "abc"), 2);
		QCOMPARE(Helper::levenshtein("c", "abc"), 2);

		QCOMPARE(Helper::levenshtein("", "abc"), 3);
		QCOMPARE(Helper::levenshtein("abc", ""), 3);
	}

};

TFW_DECLARE(Helper_Test)
