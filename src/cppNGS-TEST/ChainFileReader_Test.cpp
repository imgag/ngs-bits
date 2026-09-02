#include "TestFramework.h"
#include "ChainFileReader.h"
#include "Helper.h"
#include <iostream>
#include "Settings.h"

TEST_CLASS(ChainFileReader_Test)
{
private:

	TEST_METHOD(test01)
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		double allowed_deletion =  0.05;

		ChainFileReader r(chain_file, allowed_deletion);

		QSharedPointer<QFile> bed = Helper::openFileForReading(TESTDATA("data_in/ChainFileReader_in1.bed"));
		QSharedPointer<QFile> expected = Helper::openFileForReading(TESTDATA("data_out/ChainFileReader_out1_lifted.bed"));
		QSharedPointer<QFile> unmapped = Helper::openFileForReading(TESTDATA("data_out/ChainFileReader_out1_unmapped.bed"));

		while (! bed->atEnd())
		{

			BedLine bed_line = BedLine::fromString(bed->readLine().trimmed());
			BedLine actual;

			try
			{
				//lift is one based
				actual = r.lift(bed_line.chr(), bed_line.start()+1, bed_line.end());
			}
			catch (Exception& e)
			{
				QByteArray unmapped_line = unmapped->readLine().trimmed();
				S_EQUAL(bed_line.toString(false), unmapped_line)
				continue;

			}
			//revert back to 0-based:
			actual.setStart(actual.start()-1);
			// test that expected and lifted are equal:
			S_EQUAL(actual.toString(false), expected->readLine().trimmed());
		}
	}

	TEST_METHOD(test02)
	{
		QString chain_file = Settings::string("liftover_hg19_hg38", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg19->hg38 chain file!");

		double allowed_deletion = 0.05;

		ChainFileReader r(chain_file, allowed_deletion);

		QSharedPointer<QFile> bed = Helper::openFileForReading(TESTDATA("data_in/ChainFileReader_in2.bed"));
		QSharedPointer<QFile> out = Helper::openFileForReading(TESTDATA("data_out/ChainFileReader_out2.bed"));
		int line = 0;
		while (! bed->atEnd())
		{
			line ++;
			QByteArray bed_line = bed->readLine().trimmed();
			QByteArray out_line = out->readLine().trimmed();

			try
			{
				QList<QByteArray> bed_parts = bed_line.split('\t');
				QByteArray chr = bed_parts[0];
				int start = bed_parts[1].toInt();
				int end = bed_parts[2].toInt();

				BedLine lifted = r.lift(chr, start, end);
				lifted.setStart(lifted.start());

				QList<QByteArray> out_parts = out_line.split('\t');
				QByteArray out_chr = out_parts[0];
				int out_start = out_parts[1].toInt();
				int out_end = out_parts[2].toInt();

				S_EQUAL(QString(lifted.chr().str()), QString(out_chr));
				I_EQUAL(lifted.start(), out_start);
				I_EQUAL(lifted.end(), out_end);
			}
			catch (ArgumentException e)
			{
				std::cout << "Encountered Argument exception." << e.message().toStdString() << "\n";
			}
		}
	}

	TEST_METHOD(test03)
	{
		QString chain_file = Settings::string("liftover_hg38_hg19", true);
		if (chain_file=="") SKIP("Test needs the liftOver hg38->hg19 chain file!");

		QList<double> allowed_deletion{0, 0.05, 0.1, 0.2};
		QList<int> expected{0, 5, 10, 20};

		QSharedPointer<QFile> bed = Helper::openFileForReading(TESTDATA("data_in/ChainFileReader_in3.bed"));

		for(int i=0; i<allowed_deletion.size(); i++)
		{
			ChainFileReader r(chain_file, allowed_deletion[i]);
			bed->seek(0);
			int count  = 0;
			while (! bed->atEnd())
			{

				BedLine bed_line = BedLine::fromString(bed->readLine().trimmed());
				BedLine actual;

				try
				{
					actual = r.lift(bed_line.chr(), bed_line.start()+1, bed_line.end());
					count++;
				}
				catch (Exception& e)
				{
					continue;
				}
			}

			I_EQUAL(count, expected[i]);
		}
	}

	TEST_METHOD(single_chain_at_end_of_file)
	{
		QString chain_file = Helper::tempFileName(".chain");
		QSharedPointer<QFile> file = Helper::openFileForWriting(chain_file);
		file->write("chain 1 chr1 100 + 0 10 chr2 100 + 20 30 1\n10\n");
		file->close();

		ChainFileReader reader(chain_file, 0.0);
		BedLine lifted = reader.lift("chr1", 1, 10);
		S_EQUAL(lifted.chr().strNormalized(true), "chr2");
		I_EQUAL(lifted.start(), 21);
		I_EQUAL(lifted.end(), 30);
		QFile::remove(chain_file);
	}
};
