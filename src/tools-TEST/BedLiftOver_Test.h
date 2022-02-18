#include "TestFramework.h"

TEST_CLASS(BedLiftOver_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		EXECUTE("BedLiftOver", "-in " + TESTDATA("../cppNGS-TEST/data_in/ChainFileReader_in1.bed") + " -chain hg38_hg19 -lifted out/BedLiftOver_out1_lifted.bed -unmapped out/BedLiftOver_out1_unmapped.bed -del 0.05");
		REMOVE_LINES("out/BedLiftOver_out1_unmapped.bed", QRegExp("#"));
		COMPARE_FILES("out/BedLiftOver_out1_lifted.bed", TESTDATA("../cppNGS-TEST/data_out/ChainFileReader_out1_lifted.bed"));
		COMPARE_FILES("out/BedLiftOver_out1_unmapped.bed", TESTDATA("../cppNGS-TEST/data_out/ChainFileReader_out1_unmapped.bed"));
	}
	
	void test_02()
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		QList<double> allowed_deletion{0, 0.05, 0.1, 0.2};
		QList<int> expected{0, 5, 10, 20};

		for (int i=0; i<4; i++)
		{
			EXECUTE("BedLiftOver", "-in " + TESTDATA("../cppNGS-TEST/data_in/ChainFileReader_in3.bed") + " -chain hg38_hg19 -lifted out/BedLiftOver_out3_lifted.bed -unmapped out/BedLiftOver_out3_unmapped.bed -del " + QString::number(allowed_deletion[i], 'f'));
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
};
