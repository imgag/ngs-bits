#include "TestFramework.h"
#include "ChainFileReader.h"
#include "Helper.h"
#include <iostream>

TEST_CLASS(ChainFileReader_Test)
{
Q_OBJECT
private slots:

	void singlePosTests()
	{
		QString hg19_to_hg38 = "W:/share/opt/liftOver/hg19ToHg38.over.chain";
		ChainFileReader r;
		r.load(hg19_to_hg38);

		GenomePosition pos = GenomePosition("chr1", 123456);
		GenomePosition lifted = r.lift(pos);
		std::cout << lifted.toString().toStdString() << "\n";
		S_EQUAL(lifted.chr, "chr1");
		I_EQUAL(lifted.pos, 123456);

	}

	void developmentTest()
	{
		QString hg37_to_hg38 = "C:/Users/ahott1a1/data/liftOver/GRCh37_to_GRCh38.chain";
		ChainFileReader r;
		r.load(hg37_to_hg38);

//		QList<QByteArray> keys = r.refChromSizes().keys();
//		std::sort(keys.begin(), keys.end());
//		foreach(const QByteArray& chr, keys)
//		{
//			std::cout << chr.toStdString() << ":\t" << r.refChromSizes()[chr] << "\n";
//		}

		QSharedPointer<QFile> bed = Helper::openFileForReading(TESTDATA("data_in/ChainFileReader_test_in1.bed"));
		QSharedPointer<QFile> out = Helper::openFileForReading(TESTDATA("data_out/ChainFileReader_test_out1.bed"));

		while (! bed->atEnd())
		{
			QByteArray bed_line = bed->readLine();
			QByteArray out_line = out->readLine();


			try
			{
				QList<QByteArray> bed_parts = bed_line.split('\t');
				QByteArray chr = bed_parts[0];
				int start = bed_parts[1].toInt();
				int end = bed_parts[2].toInt();

				GenomePosition start_lifted = r.lift(GenomePosition(chr, start));
				GenomePosition end_lifted = r.lift(GenomePosition(chr, end));

				QList<QByteArray> out_parts = out_line.split('\t');
				QByteArray out_chr = out_parts[0];
				int out_start = out_parts[1].toInt();
				int out_end = out_parts[2].toInt();

				//S_EQUAL(QString(start_lifted.chr), QString(out_chr));
				I_EQUAL(start_lifted.pos, out_start);
				I_EQUAL(end_lifted.pos, out_end);
			}
			catch (ArgumentException e)
			{
				std::cout << "Encountered Argument exception.\n";
			}
		}
	}

};
