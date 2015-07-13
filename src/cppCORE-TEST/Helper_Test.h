#include "TestFramework.h"
#include "Helper.h"

TEST_CLASS(Helper_Test)
{
Q_OBJECT
private slots:

	void randomNumber()
	{
		for (int i=0; i<1000; ++i)
		{
			double random = Helper::randomNumber(-1, 1);
			IS_TRUE(random>=-1);
			IS_TRUE(random<=1);
		}
	}

	void randomString()
	{
		I_EQUAL(Helper::randomString(8).count(), 8);
		I_EQUAL(Helper::randomString(3, "AB").count(), 3);
	}

	void loadTextFile()
	{
		QStringList genes;
		genes = Helper::loadTextFile(TESTDATA("data_in/genes.txt"), true, '#', true);
		S_EQUAL(genes[0], QString("BRAF"));
		S_EQUAL(genes[1], QString("KRAS"));
		S_EQUAL(genes[2], QString("BRCA1"));
		S_EQUAL(genes[3], QString("BRCA2"));
		I_EQUAL(genes.count(), 4);

		genes = Helper::loadTextFile(TESTDATA("data_in/genes.txt"), false, '#', true);
		S_EQUAL(genes[0], QString("BRAF"));
		S_EQUAL(genes[1], QString("KRAS"));
		S_EQUAL(genes[2], QString(" "));
		S_EQUAL(genes[3], QString("BRCA1"));
		S_EQUAL(genes[4], QString("BRCA2"));
		I_EQUAL(genes.count(), 5);

		genes = Helper::loadTextFile(TESTDATA("data_in/genes.txt"), true, QChar::Null, false);
		S_EQUAL(genes[0], QString("#header"));
		S_EQUAL(genes[1], QString("BRAF"));
		S_EQUAL(genes[2], QString("KRAS"));
		S_EQUAL(genes[3], QString(""));
		S_EQUAL(genes[4], QString("BRCA1"));
		S_EQUAL(genes[5], QString("BRCA2"));
		I_EQUAL(genes.count(), 6);
	}

	void storeTextFile()
	{
		QStringList genes;
		genes << "#header" << "Gene1" << "Gene2" << "Gene3";
		Helper::storeTextFile("out/Helper_storeTextFile_out.txt", genes);
		COMPARE_FILES("out/Helper_storeTextFile_out.txt", TESTDATA("data_out/Helper_storeTextFile_out.txt"));
	}

	void levenshtein()
	{
		I_EQUAL(Helper::levenshtein("", ""), 0);
		I_EQUAL(Helper::levenshtein("abc", "abc"), 0);

		I_EQUAL(Helper::levenshtein("abc", "xbc"), 1);
		I_EQUAL(Helper::levenshtein("abc", "axc"), 1);
		I_EQUAL(Helper::levenshtein("abc", "abx"), 1);
		I_EQUAL(Helper::levenshtein("xbc", "abc"), 1);
		I_EQUAL(Helper::levenshtein("axc", "abc"), 1);
		I_EQUAL(Helper::levenshtein("abx", "abc"), 1);

		I_EQUAL(Helper::levenshtein("abc", "a"), 2);
		I_EQUAL(Helper::levenshtein("abc", "b"), 2);
		I_EQUAL(Helper::levenshtein("abc", "c"), 2);
		I_EQUAL(Helper::levenshtein("a", "abc"), 2);
		I_EQUAL(Helper::levenshtein("b", "abc"), 2);
		I_EQUAL(Helper::levenshtein("c", "abc"), 2);

		I_EQUAL(Helper::levenshtein("", "abc"), 3);
		I_EQUAL(Helper::levenshtein("abc", ""), 3);
	}

};
