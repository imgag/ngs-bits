#include "TestFramework.h"
#include "ChainFileReader.h"
#include "Helper.h"
#include "VcfFile.h"
#include <iostream>
#include "HttpRequestHandler.h"
#include <QTime>

TEST_CLASS(ChainFileReader_Test)
{
Q_OBJECT
private slots:

	void index()
	{
		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		ChainFileReader r;
		r.load(hg38_to_hg19);

		QHash<Chromosome, QList<GenomicAlignment>> chromosomes_list = r.chromosomes_list;

		QList<GenomicAlignment> alignments = chromosomes_list[Chromosome("chr1")];

		GenomicAlignment first = alignments[0];

//		for (int i=0; i< first.alignment.size(); i++)
//		{
//			if (i % first.index_frequency == 0)
//			{
//				std::cout << "IndexLine:  start: " << first.index[i+1/freq]
//			}
//		}
	}

	void createTestBed()
	{
		QSharedPointer<QFile> out = Helper::openFileForWriting("C:/Users/ahott1a1/data/liftOver/test_regions.bed");
		BedLine line;
		line.setChr("chr1");
//		int border = 177417;
		int border = 120461015-100;
		int fragment_size = 20;

		int prev_start = border-fragment_size;

		for (int i=0; i<201; i++)
		{
			line.setStart(prev_start+1);
			line.setEnd(line.start()+fragment_size);

			prev_start = line.start();
			out->write(line.toString(false).toLatin1() + "\n");
		}
	}

	void wrong()
	{
		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		ChainFileReader r;
		r.load(hg38_to_hg19);

		int count = 0;
		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/wrongly_mapped1.bed");
		while (! bed->atEnd())
		{
			count ++;

//			if (count > 1)
//			{
//				break;
//			}

			bool expected_error = false;
			bool actual_error = false;

			BedLine bed_line = BedLine::fromString(bed->readLine().trimmed());
			BedLine actual;
			BedLine expected;
			try
			{
				actual = r.lift_list(bed_line.chr(), bed_line.start(), bed_line.end());
			}
			catch (Exception& e)
			{
				std::cout << e.message().toStdString() << "\n";
				actual_error = true;
			}

			try
			{
				expected = liftOver(bed_line.chr(), bed_line.start(), bed_line.end(), false);
			}
			catch (Exception& e)
			{

//				std::cout << e.message().toStdString() << "\n";
				expected_error = true;
			}

			if (actual_error && expected_error)
			{
				// both throw error: ok
				std::cout << "correct\n";
				continue;
			}

			if (actual_error || expected_error)
			{
				// only one throws
				if (actual_error)
				{
					std::cout << "My lift throws error!\n";
				}
				if (expected_error)
				{
					std::cout << "Only Server throws error!\n";
				}

			}

			if (actual.chr() != expected.chr() || actual.start() != expected.start() || actual.end() != expected.end())
			{
				std::cout << "To lift: \t" << bed_line.toString(true).toStdString() << "\n";
				std::cout << "expected:\t" << expected.toString(true).toStdString() << "\n";
				std::cout << "actual:  \t" << actual.toString(true).toStdString() << "\n\n";
			} else
			{
				std::cout << "correct\n";
			}
		}
	}

	void dev()
	{
		QTime timer;
		timer.start();
		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		ChainFileReader r;
		r.load(hg38_to_hg19);

		std::cout << "Loading took: " << timer.elapsed() / 1000.0 << "s.\n";
		timer.start();


		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_zero_based.bed");
		QSharedPointer<QFile> expected = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_zero_based_lifted_to_hg19_REF.bed");
//		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var.bed");
//		QSharedPointer<QFile> expected = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_lifted_to_hg19_REF.bed");


		QSharedPointer<QFile> out = Helper::openFileForWriting("C:/Users/ahott1a1/data/liftOver/wrongly_mapped.bed");
//		QSharedPointer<QFile> lifted = Helper::openFileForWriting("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_lifted.bed");

		int count = 0;
		BedLine last;
		while (! bed->atEnd())
		{
			count++;

			if(count % 500000 == 0)
			{
				std::cout << "Line: " << count << "\n";
			}

			BedLine bed_line = BedLine::fromString(bed->readLine().trimmed());

			BedLine actual;

			try
			{
				actual = r.lift_list(bed_line.chr(), bed_line.start(), bed_line.end());
			}
			catch (...)
			{
				continue;
			}
			BedLine ref = BedLine::fromString(expected->readLine());

			if (ref.chr() != actual.chr() || ref.start() != actual.start() || ref.end() != actual.end())
			{
				std::cout << "line: " << count << "  - Mismatch!\n";
				std::cout << "to lift:  " << bed_line.toString(true).toStdString() << "\n";
				std::cout << "lifted :  " << actual.toString(true).toStdString() << "\n";
				std::cout << "expected: " << ref.toString(true).toStdString() << "\n";

				out->write(last.toString(false).toLatin1() + "\n");
				out->write(bed_line.toString(false).toLatin1() + "\n");
				break;

			}

//			lifted->write(actual.toString(false).toLatin1() + "\n");
			last = bed_line;

		}
		std::cout << "Lifting took: " << timer.elapsed() / 1000.0 << "s.\n";
	}

	void time_it()
	{
		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		ChainFileReader r;
		r.load(hg38_to_hg19);

		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_zero_based.bed");

		QTime timer;

		BedLine tree_lifted;
		bool tree_error = false;
		BedLine list_lifted;
		bool list_error = false;

		bed->seek(0);
		timer.start();
		while (! bed->atEnd())
		{
			QByteArray bed_line = bed->readLine().trimmed();

			QList<QByteArray> bed_parts = bed_line.split('\t');
			QByteArray chr = bed_parts[0];
			int start = bed_parts[1].toInt()-1;
			int end = bed_parts[2].toInt();


			try
			{
				list_lifted = r.lift_list(Chromosome(chr), start, end);
			} catch(Exception& e)
			{
				//std::cout << e.message().toStdString() << "\n";
				list_error = true;
			}
		}
		std::cout << "Naive list: " << timer.elapsed()/1000.0 << "s \n";


		bed->seek(0);
		timer.start();
		while (! bed->atEnd())
		{
			QByteArray bed_line = bed->readLine().trimmed();

			QList<QByteArray> bed_parts = bed_line.split('\t');
			QByteArray chr = bed_parts[0];
			int start = bed_parts[1].toInt()-1;
			int end = bed_parts[2].toInt();

			try
			{
				tree_lifted = r.lift_tree(Chromosome(chr), start, end);
				//std::cout << tree_lifted.toString(true).toStdString() << "\n";
			} catch(Exception& e)
			{
				//std::cout << e.message().toStdString() << "\n";
				tree_error = true;
			}
		}
		std::cout << "Tree: " << timer.elapsed()/1000.0 << "s \n";

		bed->seek(0);
		timer.start();
		while (! bed->atEnd())
		{
			QByteArray bed_line = bed->readLine().trimmed();

			QList<QByteArray> bed_parts = bed_line.split('\t');
			QByteArray chr = bed_parts[0];
			int start = bed_parts[1].toInt()-1;
			int end = bed_parts[2].toInt();

			try
			{
				tree_lifted = r.lift_tree(Chromosome(chr), start, end);
				//std::cout << tree_lifted.toString(true).toStdString() << "\n";
			} catch(Exception& e)
			{
				tree_error = true;
			}
			try
			{
				list_lifted = r.lift_list(Chromosome(chr), start, end);
			} catch(Exception& e)
			{
				list_error = true;
			}

			if (list_error && tree_error)
			{
				continue;
			}

			if (tree_lifted.chr() != list_lifted.chr() || tree_lifted.start() != list_lifted.start() || tree_lifted.end() != list_lifted.end())
			{
				std::cout << "Lifting between tree and list differs!\n";
			}


		}
		std::cout << "Comparision: " << timer.elapsed()/1000.0 << "s \n";
	}

	void generalDatastructureTest()
	{
		QString hg19_to_hg38 = "C:/Users/ahott1a1/data/liftOver/hg19ToHg38_part.over.chain";
		ChainFileReader r;
		r.load(hg19_to_hg38);

		BedLine lifted = r.lift_tree("chrY", 1000000+10, 1000000+10);
		S_EQUAL(lifted.chr().str(), "chrY");
		I_EQUAL(lifted.start(), 2000000+10);

		lifted = r.lift_tree("chrY", 1000000+1126, 1000000+1126);
		S_EQUAL(lifted.chr().str(), "chrY");
		I_EQUAL(lifted.start(), 2000000 + 1112);

		IS_THROWN(ArgumentException, r.lift_tree("chrY", 1000000+510, 1000000+510));

		lifted = r.lift_tree("chrY", 1000000+1330, 1000000+1330);
		S_EQUAL(lifted.chr().str(), "chrX");
		I_EQUAL(lifted.start(), 3000000 + 4);

		lifted = r.lift_tree("chrY", 1001326 + 28, 1001326 + 28);
		S_EQUAL(lifted.chr().str(), "chrX");
		I_EQUAL(lifted.start(), 3000000 + 29);
	}

	void test_bed_hg19Tohg38()
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
		QSharedPointer<QFile> out = Helper::openFileForReading(TESTDATA("data_out/ChainFileReader_Test_out1.bed"));
		int line = 0;
		while (! bed->atEnd())
		{
			line ++;
			QByteArray bed_line = bed->readLine().trimmed();
			QByteArray out_line = out->readLine().trimmed();

			try
			{
//				std::cout << "line:\t" << line <<"\n";
				QList<QByteArray> bed_parts = bed_line.split('\t');
				QByteArray chr = bed_parts[0];
				int start = bed_parts[1].toInt()-1;
				int end = bed_parts[2].toInt();

				BedLine lifted = r.lift_tree(chr, start, end);
				lifted.setStart(lifted.start() +1);

				QList<QByteArray> out_parts = out_line.split('\t');
				QByteArray out_chr = out_parts[0];
				int out_start = out_parts[1].toInt();
				int out_end = out_parts[2].toInt();

				if (QString(lifted.chr().str()) != QString(out_chr) || lifted.start() != out_start || lifted.end() != out_end)
				{
					std::cout << "To lift:   " + bed_line.toStdString() + "\n";
					std::cout << "lifted to: " + lifted.toString(true).toStdString() + "\n";
					std::cout << "expected:  " + out_line.toStdString() + "\n";
				}
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

//	void test_bed_hg38Tohg19()
//	{
//		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
//		ChainFileReader r;
//		r.load(hg38_to_hg19);

//		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/NA12878_45_var.bed");
//		QSharedPointer<QFile> out = Helper::openFileForReading("C:/Users/ahott1a1/data/NA12878_45_var_lifted_with_server.bed");
//		int line = 0;
//		while (! out->atEnd())
//		{
//			line ++;
//			QByteArray bed_line = bed->readLine().trimmed();
//			QByteArray out_line = out->readLine().trimmed();

//			QList<QByteArray> bed_parts = bed_line.split('\t');
//			QByteArray chr = bed_parts[0];
//			int start = bed_parts[1].toInt();
//			int end = bed_parts[2].toInt();
//			BedLine lifted;

//			try
//			{
////				std::cout << "line:\t" << line <<"\n";
//				lifted = r.lift(chr, start, end);

//			}
//			catch (ArgumentException e)
//			{
//				if ( ! out_line.contains("ERROR"))
//				{
//					std::cout << "To lift: " + bed_line.toStdString() + "\n";
//					THROW(ArgumentException, "Code threw error but shouldn't have:" + e.message())
//				}
//				else
//				{
//					continue;
//					// fine both threw error
//				}
//			}

//			if (out_line.contains("ERROR"))
//			{
//				std::cout << "To lift:   " + bed_line.toStdString() + "\n";
//				std::cout << "lifted to: " + lifted.toString(true).toStdString() + "\n";
//				THROW(ArgumentException, "Code should have thrown an error: " + out_line)
//			}

//			QList<QByteArray> out_parts = out_line.split('\t');
//			QByteArray out_chr = out_parts[0];
//			int out_start = out_parts[1].toInt();
//			int out_end = out_parts[2].toInt();

//			if (QString(lifted.chr().str()) != QString(out_chr) || lifted.start() != out_start || lifted.end() != out_end)
//			{
//				std::cout << "To lift:   " + bed_line.toStdString() + "\n";
//				std::cout << "lifted to: " + lifted.toString(true).toStdString() + "\n";
//				std::cout << "expected:  " + out_line.toStdString() + "\n";
//			}

//			S_EQUAL(QString(lifted.chr().str()), QString(out_chr));
//			I_EQUAL(lifted.start(), out_start);
//			I_EQUAL(lifted.end(), out_end);
//		}

//	}

//	void writeTestFileFromServer()
//	{
//		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/NA12878_45_var.bed");
//		QSharedPointer<QFile> out = Helper::openFileForWriting("C:/Users/ahott1a1/data/NA12878_45_var_lifted_with_server2.bed");

////		QSharedPointer<QFile> bed = Helper::openFileForReading(TESTDATA("data_in/ChainFileReader_test_in1.bed"));
////		QSharedPointer<QFile> out = Helper::openFileForWriting("C:/Users/ahott1a1/data/ChainFileReader_test_out2.bed");
//		bool hg19_to_hg38 = true;
//		int line =0;
//		while (! bed->atEnd())
//		{
//			if (line % 100 == 0)
//			{
//				std::cout << "Writing line " << line << "\n";
//			}
//			line++;
//			QByteArray bed_line = bed->readLine().trimmed();
//			QList<QByteArray> bed_parts = bed_line.split('\t');

//			BedLine in = BedLine();
//			in.setChr(Chromosome(bed_parts[0]));
//			in.setStart(bed_parts[1].toInt());
//			in.setEnd(bed_parts[2].toInt());

//			BedLine out_bed;
//			try
//			{
//				out_bed = liftOver(in.chr(), in.start(), in.end(), hg19_to_hg38);
//			}
//			catch (ArgumentException e)
//			{
//				out->write("ERROR\t" + e.message().toLatin1() + "\t" + in.toString(true).toLatin1() + "\n");
//				continue;
//			}

//			out->write(out_bed.toString(false).toLatin1() + "\n");
//		}
//	}


	void serverTesting()
	{

		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		bool hg19_to_hg38 = false;
		ChainFileReader r;
		r.load(hg38_to_hg19);

//		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var.bed");
//		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_zero_based.bed");
		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/test_regions.bed");

		QSharedPointer<QFile> out = Helper::openFileForWriting("C:/Users/ahott1a1/data/liftOver/wrongly_mapped.bed");

		int line = 0;
		int wrong = 0;
		int error = 0;
		int correct =0;
		while (! bed->atEnd())
		{
			if (line%50 == 0)
			{
				std::cout << "line:" << line << "\n";
				std::cout << "correct: " << correct << "\n";
				std::cout << "wrong: " << wrong << "\n";
				std::cout << "error: " << error << "\n";
			}

			line++;
			QByteArray bed_line = bed->readLine().trimmed();
			QList<QByteArray> bed_parts = bed_line.split('\t');

			BedLine in = BedLine();
			in.setChr(Chromosome(bed_parts[0]));
			in.setStart(bed_parts[1].toInt());
			in.setEnd(bed_parts[2].toInt());
			BedLine lifted;
			BedLine out_bed;

			ArgumentException err = ArgumentException("Placeholder Error", "ChainFileReader_Test.h", 371);
			bool actual_error = false;
			try
			{
//				lifted = r.lift_list(in.chr(), in.start()-1, in.end());
				lifted = r.lift_list(in.chr(), in.start(), in.end());

			}
			catch (ArgumentException e)
			{
				err = e;
				actual_error = true;
			}
//			lifted.setStart(lifted.start() +1);

			try
			{
				out_bed = liftOver(in.chr(), in.start(), in.end(), hg19_to_hg38);
			}
			catch (ArgumentException e)
			{
				if (actual_error)
				{
					// both threw error: ok
					std::cout << "both error.\n";
					correct++;
					continue;
				}
				else
				{
					wrong++;
					std::cout << "Only server threw error.... " << e.message().toStdString() << "\n";
					std::cout << "mine lifted: " << lifted.toString(true).toStdString() <<"\n";
					out->write(bed_line + "\tserver threw error on my side lifted: " + e.message().toLatin1() + "\n");
					out->flush();
					continue;
				}
			}

			if (actual_error)
			{
				error++;
				std::cout << "Only my side encountered Argument exception." << err.message().toStdString() << "\n";
				out->write(bed_line + "\tunexpected error on my side:" + err.message().toLatin1() + "\n");
				out->flush();
			}

			if (lifted.chr() != out_bed.chr() || lifted.start() != out_bed.start() || lifted.end() != out_bed.end())
			{
				wrong++;
				std::cout << "not the same coordinates!\n";
				std::cout << "expected: " << out_bed.toString(true).toStdString() << "\n";
				std::cout << "actual: " << lifted.toString(true).toStdString() << "\n";
				out->write(bed_line + "\tmapped to different coordinates!\n");
				out->flush();
				continue;
			}
			correct++;
		}
	}

private:
	BedLine liftOver(const Chromosome& chr, int start, int end, bool hg19_to_hg38)
	{
		//special handling of chrMT (they are the same for GRCh37 and GRCh38)
		if (chr.strNormalized(true)=="chrMT") return BedLine(chr, start, end);

		//call lift-over webservice
		QString url = "https://portal.img.med.uni-tuebingen.de/LiftOver/liftover.php?chr=" + chr.strNormalized(true) + "&start=" + QString::number(start) + "&end=" + QString::number(end);
		if (!hg19_to_hg38) url += "&dir=hg38_hg19";
		QString output = HttpRequestHandler(HttpRequestHandler::ProxyType::NONE).get(url);

		//handle error from webservice
		if (output.contains("ERROR")) THROW(ArgumentException, "genomic coordinate lift-over failed: " + output);

		//convert output to region
		BedLine region = BedLine::fromString(output);
		if (!region.isValid()) THROW(ArgumentException, "genomic coordinate lift-over failed: Could not convert output '" + output + "' to valid region");

		return region;
	}




};
