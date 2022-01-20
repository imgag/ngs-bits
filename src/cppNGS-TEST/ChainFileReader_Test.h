#include "TestFramework.h"
#include "ChainFileReader.h"
#include "Helper.h"
#include <iostream>

TEST_CLASS(ChainFileReader_Test)
{
Q_OBJECT
private slots:

	void test()
	{
		QString hg37_to_hg38 = "C:/Users/ahott1a1/data/liftOver/hg19ToHg38.over.chain";
		ChainFileReader r;
		r.load(hg37_to_hg38);

	}

	void generalDatastructureTest()
	{
		QString hg19_to_hg38 = "C:/Users/ahott1a1/data/liftOver/hg19ToHg38_part.over.chain";
		ChainFileReader r;
		r.load(hg19_to_hg38);

		GenomePosition pos = GenomePosition("chrY", 1000000+10);
		GenomePosition lifted = r.lift(pos);
		S_EQUAL(lifted.chr, "chrY");
		I_EQUAL(lifted.pos, 2000000+10);

		pos = GenomePosition("chrY", 1000000+1126);
		lifted = r.lift(pos);
		S_EQUAL(lifted.chr, "chrY");
		I_EQUAL(lifted.pos, 2000000 + 1112);


		pos = GenomePosition("chrY", 1000000+510);
		IS_THROWN(ArgumentException, r.lift(pos));

		pos = GenomePosition("chrY", 1000000+1330);
		lifted = r.lift(pos);
		S_EQUAL(lifted.chr, "chrX");
		I_EQUAL(lifted.pos, 3000000 + 4);

		pos = GenomePosition("chrY", 1001326 + 28);
		lifted = r.lift(pos);
		S_EQUAL(lifted.chr, "chrX");
		I_EQUAL(lifted.pos, 3000000 + 29);
	}

	void developmentTest()
	{
		QString hg37_to_hg38 = "C:/Users/ahott1a1/data/liftOver/hg19ToHg38.over.chain";
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
		int line = 0;
		while (! bed->atEnd())
		{
			line ++;
			QByteArray bed_line = bed->readLine();
			QByteArray out_line = out->readLine();


			try
			{
//				std::cout << "line:\t" << line <<"\n";
				QList<QByteArray> bed_parts = bed_line.split('\t');
				QByteArray chr = bed_parts[0];
				int start = bed_parts[1].toInt();
				int end = bed_parts[2].toInt();

				GenomePosition start_lifted = r.lift(GenomePosition(chr, start));
				GenomePosition end_lifted = r.lift(GenomePosition(chr, end));

				if (start_lifted.pos > end_lifted.pos)
				{
					GenomePosition tmp = start_lifted;
					start_lifted = end_lifted;
					end_lifted = tmp;
				}

				QList<QByteArray> out_parts = out_line.split('\t');
				QByteArray out_chr = out_parts[0];
				int out_start = out_parts[1].toInt();
				int out_end = out_parts[2].toInt();

//				std::cout << "lifted_start:" << start_lifted.chr.toStdString() << ":\t" << start_lifted.pos << "\n";
				S_EQUAL(QString(start_lifted.chr), QString(out_chr));
				I_EQUAL(start_lifted.pos, out_start);
				I_EQUAL(end_lifted.pos, out_end);
			}
			catch (ArgumentException e)
			{
				std::cout << "Encountered Argument exception." << e.message().toStdString() << "\n";
			}
		}
	}
};
