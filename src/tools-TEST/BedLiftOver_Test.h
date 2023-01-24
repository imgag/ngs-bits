#include "TestFramework.h"

TEST_CLASS(BedLiftOver_Test)
{
Q_OBJECT
private slots:
	
	// a collection of regions in edge cases
	void test_01()
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		EXECUTE("BedLiftOver", "-in " + TESTDATA("../cppNGS-TEST/data_in/ChainFileReader_in1.bed") + " -chain hg38_hg19 -out out/BedLiftOver_out1_lifted.bed -unmapped out/BedLiftOver_out1_unmapped.bed -max_deletion 5 -max_increase 9999");
		REMOVE_LINES("out/BedLiftOver_out1_lifted.bed", QRegExp("#"));
		REMOVE_LINES("out/BedLiftOver_out1_unmapped.bed", QRegExp("#"));
		COMPARE_FILES("out/BedLiftOver_out1_lifted.bed", TESTDATA("../cppNGS-TEST/data_out/ChainFileReader_out1_lifted.bed"));
		COMPARE_FILES("out/BedLiftOver_out1_unmapped.bed", TESTDATA("../cppNGS-TEST/data_out/ChainFileReader_out1_unmapped.bed"));
	}
	
	// testing allowed number of deleted bases
	void test_02()
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		QList<int> allowed_deletion{0, 5, 10, 20};
		QList<int> expected{0, 5, 10, 20};

		for (int i=0; i<allowed_deletion.size(); i++)
		{
			EXECUTE("BedLiftOver", "-in " + TESTDATA("../cppNGS-TEST/data_in/ChainFileReader_in3.bed") + " -chain hg38_hg19 -out out/BedLiftOver_out3_lifted.bed -unmapped out/BedLiftOver_out3_unmapped.bed -max_deletion " + QString::number(allowed_deletion[i]));
			REMOVE_LINES("out/BedLiftOver_out3_lifted.bed", QRegExp("#"));
			REMOVE_LINES("out/BedLiftOver_out3_unmapped.bed", QRegExp("#"));

			QFile lifted("out/BedLiftOver_out3_lifted.bed");
			lifted.open(QFile::ReadOnly | QIODevice::Text);
			int count=0;
			while(! lifted.atEnd())
			{
				count++;
				lifted.readLine();
			}

			I_EQUAL(count, expected[i]);

			QFile unmapped("out/BedLiftOver_out3_unmapped.bed");
			unmapped.open(QFile::ReadOnly | QIODevice::Text);
			count=0;
			while(! unmapped.atEnd())
			{
				count++;
				unmapped.readLine();
			}
			int expected_unmapped = 22-expected[i];
			I_EQUAL(count, expected_unmapped)
		}
	}

	// special chromosomes
	void test_03()
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		EXECUTE("BedLiftOver", "-in " + TESTDATA("data_in/BedLiftOver_in1.bed") + " -chain hg38_hg19 -out out/BedLiftOver_out3_lifted.bed -unmapped out/BedLiftOver_out3_unmapped.bed -remove_special_chr");
		COMPARE_FILES("out/BedLiftOver_out3_unmapped.bed", TESTDATA("data_out/BedLiftOver_out3_unmapped.bed"));
	}

	//max size increase
	void test_04()
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		QList<int> max_increase{0, 9, 10, 19, 20};
		QList<int> expected{1, 1, 2, 2, 3};

		for (int i=0; i<max_increase.size(); i++)
		{
			EXECUTE("BedLiftOver", "-in " + TESTDATA("data_in/BedLiftOver_in2.bed") + " -chain hg38_hg19 -out out/BedLiftOver_out4_lifted.bed -unmapped out/BedLiftOver_out4_unmapped.bed -max_deletion 5 -max_increase " + QString::number(max_increase[i]));
			REMOVE_LINES("out/BedLiftOver_out4_lifted.bed", QRegExp("#"));
			REMOVE_LINES("out/BedLiftOver_out4_unmapped.bed", QRegExp("#"));

			QFile lifted("out/BedLiftOver_out4_lifted.bed");
			lifted.open(QFile::ReadOnly | QIODevice::Text);
			int count=0;
			while(! lifted.atEnd())
			{
				count++;
				lifted.readLine();
			}

			I_EQUAL(count, expected[i]);

			QFile unmapped("out/BedLiftOver_out4_unmapped.bed");
			unmapped.open(QFile::ReadOnly | QIODevice::Text);
			count=0;
			while(! unmapped.atEnd())
			{
				count++;
				unmapped.readLine();
			}
			int expected_unmapped = 3-expected[i];
			I_EQUAL(count, expected_unmapped)
		}
	}

	void test_annotations()
	{
		EXECUTE("BedLiftOver", "-in " + TESTDATA("../cppNGS-TEST/data_in/ChainFileReader_in2.bed") + " -chain hg19_hg38 -out out/BedLiftOver_out4_lifted.bed -unmapped out/BedLiftOver_out4_unmapped.bed");

	}
};
