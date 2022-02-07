#include "TestFramework.h"
#include "ChainFileReader.h"
#include "Helper.h"
#include "VcfFile.h"
#include <iostream>
#include "HttpRequestHandler.h"

TEST_CLASS(ChainFileReader_Test)
{
Q_OBJECT
private slots:

	void dev()
	{
		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		ChainFileReader r;
		r.load(hg38_to_hg19);

		std::cout << r.lift("chr1",	143196583, 143196585).toString(true).toStdString();

//		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var.bed");
//		QSharedPointer<QFile> out = Helper::openFileForWriting("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_lifted.bed");
//		QSharedPointer<QFile> unmapped = Helper::openFileForWriting("C:/Users/ahott1a1/data/liftOver/NA12878_45_var_unmapped.bed");
//		int count = 0;

//		while (! bed->atEnd())
//		{
//			count++;

//			if (count%1000 == 0)
//			{
//				std::cout << "Count: " << count << "\n";
//			}
//			QByteArray bed_line = bed->readLine().trimmed();
//			QList<QByteArray> bed_parts = bed_line.split('\t');
//			QByteArray chr = bed_parts[0];
//			int start = bed_parts[1].toInt();
//			int end = bed_parts[2].toInt();
//			BedLine lifted;
//			try
//			{
//				lifted = r.lift(chr, start, end);

//				GenomePosition start_lifted = r.lift(chr, start);
//				GenomePosition end_lifted = r.lift(chr, end);

//				if (start_lifted.pos > end_lifted.pos)
//				{
//					GenomePosition tmp = start_lifted;
//					start_lifted = end_lifted;
//					end_lifted = tmp;
//				}

//				BedLine single;
//				single.setChr(start_lifted.chr);
//				single.setStart(start_lifted.pos);
//				single.setEnd(end_lifted.pos);

//				if (single.chr() != lifted.chr() || single.start() != lifted.start() || single.end() != lifted.end())
//				{
//					std::cout << "lifted: " << lifted.toString(true).toStdString() << "\n";
//					std::cout << "single: " << single.toString(true).toStdString() << "\n";
//					break;
//				}
//				IS_TRUE(single == lifted)
//			}
//			catch (Exception e)
//			{
//				std::cout << "Some error: " << e.message().toStdString() << "\n";
//				unmapped->write(bed_line + "\n");
//				continue;
//			}
//			out->write(lifted.toString(false).toLatin1() + "\n");
//		}

	}

	void generalDatastructureTest()
	{
		QString hg19_to_hg38 = "C:/Users/ahott1a1/data/liftOver/hg19ToHg38_part.over.chain";
		ChainFileReader r;
		r.load(hg19_to_hg38);

		GenomePosition lifted = r.lift("chrY", 1000000+10);
		S_EQUAL(lifted.chr.str(), "chrY");
		I_EQUAL(lifted.pos, 2000000+10);

		lifted = r.lift("chrY", 1000000+1126);
		S_EQUAL(lifted.chr.str(), "chrY");
		I_EQUAL(lifted.pos, 2000000 + 1112);

		IS_THROWN(ArgumentException, r.lift("chrY", 1000000+510));

		lifted = r.lift("chrY", 1000000+1330);
		S_EQUAL(lifted.chr.str(), "chrX");
		I_EQUAL(lifted.pos, 3000000 + 4);

		lifted = r.lift("chrY", 1001326 + 28);
		S_EQUAL(lifted.chr.str(), "chrX");
		I_EQUAL(lifted.pos, 3000000 + 29);
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

				BedLine lifted = r.lift(chr, start, end);
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


	void DevelopmentTestServer()
	{

		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		bool hg19_to_hg38 = false;
		ChainFileReader r;
		r.load(hg38_to_hg19);

		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/liftOver/NA12878_45_var.bed");
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
			try
			{
				//std::cout << "line " << line <<":" << bed_line.toStdString() << "\n";
				lifted = r.lift(in.chr(), in.start()-1, in.end());
			}
			catch (ArgumentException e)
			{
				try
				{
					out_bed = liftOver(in.chr(), in.start(), in.end(), hg19_to_hg38);
				}
				catch(ArgumentException)
				{
					//std::cout << "Both throw errors fine.\n";
					correct++;
					continue;
				}
				error++;
				std::cout << "Only my side encountered Argument exception." << e.message().toStdString() << "\n";
				out->write(bed_line + "\tunexpected error on my side\n");
				out->flush();
			}
			lifted.setStart(lifted.start() +1);

			try
			{
				out_bed = liftOver(in.chr(), in.start(), in.end(), hg19_to_hg38);
			}
			catch (ArgumentException e)
			{
				wrong++;
				std::cout << "Only server threw error.... " << e.message().toStdString() << "\n";
				std::cout << "mine lifted: " << lifted.toString(true).toStdString() <<"\n";
				out->write(bed_line + "\tserver threw error on my side lifted!\n");
				out->flush();
				continue;
			}

			if (lifted.chr() != out_bed.chr() || lifted.start() != out_bed.start() || lifted.end() != out_bed.end())
			{
				wrong++;
				std::cout << "not the same coordinates!\n";
				out->write(bed_line + "\tmapped to different coordinates!\n");
				out->flush();
				continue;
			}
			correct++;
		}
		std::cout << "Positions wrongly mapped: " << wrong << "\n";
	}

private:
	BedLine liftOver(const Chromosome& chr, int start, int end, bool hg19_to_hg38)
	{
		//special handling of chrMT (they are the same for GRCh37 and GRCh38)
		if (chr.strNormalized(true)=="chrMT") return BedLine(chr, start, end);

		//convert start to BED format (0-based)
		start -= 1;

		//call lift-over webservice
		QString url = "https://portal.img.med.uni-tuebingen.de/LiftOver/liftover.php?chr=" + chr.strNormalized(true) + "&start=" + QString::number(start) + "&end=" + QString::number(end);
		if (!hg19_to_hg38) url += "&dir=hg38_hg19";
		QString output = HttpRequestHandler(HttpRequestHandler::ProxyType::NONE).get(url);

		//handle error from webservice
		if (output.contains("ERROR")) THROW(ArgumentException, "genomic coordinate lift-over failed: " + output);

		//convert output to region
		BedLine region = BedLine::fromString(output);
		if (!region.isValid()) THROW(ArgumentException, "genomic coordinate lift-over failed: Could not convert output '" + output + "' to valid region");

		//revert to 1-based
		region.setStart(region.start()+1);

		return region;
	}




};
