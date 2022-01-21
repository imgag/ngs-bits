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

	void generalDatastructureTest()
	{
		QString hg19_to_hg38 = "C:/Users/ahott1a1/data/liftOver/hg19ToHg38_part.over.chain";
		ChainFileReader r;
		r.load(hg19_to_hg38);

		GenomePosition lifted = r.lift("chrY", 1000000+10);
		S_EQUAL(lifted.chr, "chrY");
		I_EQUAL(lifted.pos, 2000000+10);

		lifted = r.lift("chrY", 1000000+1126);
		S_EQUAL(lifted.chr, "chrY");
		I_EQUAL(lifted.pos, 2000000 + 1112);

		IS_THROWN(ArgumentException, r.lift("chrY", 1000000+510));

		lifted = r.lift("chrY", 1000000+1330);
		S_EQUAL(lifted.chr, "chrX");
		I_EQUAL(lifted.pos, 3000000 + 4);

		lifted = r.lift("chrY", 1001326 + 28);
		S_EQUAL(lifted.chr, "chrX");
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
		QSharedPointer<QFile> out = Helper::openFileForReading(TESTDATA("data_out/ChainFileReader_test_out2.bed"));
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
				int start = bed_parts[1].toInt();
				int end = bed_parts[2].toInt();

				BedLine lifted = r.lift(chr, start, end);

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

	void test_bed_hg38Tohg19()
	{
		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
		ChainFileReader r;
		r.load(hg38_to_hg19);

		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/NA12878_45_var.bed");
		QSharedPointer<QFile> out = Helper::openFileForReading("C:/Users/ahott1a1/data/NA12878_45_var_lifted_with_server.bed");
		int line = 0;
		while (! out->atEnd())
		{
			line ++;
			QByteArray bed_line = bed->readLine().trimmed();
			QByteArray out_line = out->readLine().trimmed();

			QList<QByteArray> bed_parts = bed_line.split('\t');
			QByteArray chr = bed_parts[0];
			int start = bed_parts[1].toInt();
			int end = bed_parts[2].toInt();
			BedLine lifted;

			try
			{
//				std::cout << "line:\t" << line <<"\n";
				lifted = r.lift(chr, start, end);

			}
			catch (ArgumentException e)
			{
				if ( ! out_line.contains("ERROR"))
				{
					std::cout << "To lift: " + bed_line.toStdString() + "\n";
					THROW(ArgumentException, "Code threw error but shouldn't have:" + e.message())
				}
				else
				{
					continue;
					// fine both threw error
				}
			}

			if (out_line.contains("ERROR"))
			{
				std::cout << "To lift:   " + bed_line.toStdString() + "\n";
				std::cout << "lifted to: " + lifted.toString(true).toStdString() + "\n";
				THROW(ArgumentException, "Code should have thrown an error: " + out_line)
			}

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

	}

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


//	void DevelopmentTestServer()
//	{

//		QString hg38_to_hg19 = "C:/Users/ahott1a1/data/liftOver/hg38ToHg19.over.chain";
//		bool hg19_to_hg38 = false;
//		ChainFileReader r;
//		r.load(hg38_to_hg19);

//		QSharedPointer<QFile> bed = Helper::openFileForReading("C:/Users/ahott1a1/data/NA12878_45_var.bed");

//		int line = 0;
//		int wrong = 0;
//		int error = 0;
//		int correct =0;
//		while (! bed->atEnd())
//		{
//			if (line%10 == 0)
//			{
//				std::cout << "line:" << line << "\n";
//			}
//			line++;
//			QByteArray bed_line = bed->readLine().trimmed();
//			QList<QByteArray> bed_parts = bed_line.split('\t');

//			BedLine in = BedLine();
//			in.setChr(Chromosome(bed_parts[0]));
//			in.setStart(bed_parts[1].toInt());
//			in.setEnd(bed_parts[2].toInt());
//			GenomePosition start_lifted("", -1);
//			GenomePosition end_lifted("", -1);
//			try
//			{
//				//std::cout << "line " << line <<":" << bed_line.toStdString() << "\n";
//				start_lifted = r.lift(GenomePosition(in.chr().str(), in.start()));
//				end_lifted = r.lift(GenomePosition(in.chr().str(), in.end()));
//			}
//			catch (ArgumentException e)
//			{
//				try
//				{
//					BedLine out_bed = liftOver(in.chr(), in.start(), in.end(), hg19_to_hg38);
//				}
//				catch(ArgumentException e)
//				{
//					std::cout << "Both throw errors fine.\n";
//					continue;
//				}
//				error++;
////				std::cout << chr.toStdString() << ":" << start << "-" << end << "\n";
//				std::cout << "Only my side encountered Argument exception." << e.message().toStdString() << "\n";
//			}

//			if (start_lifted.pos > end_lifted.pos)
//			{
//				GenomePosition tmp = start_lifted;
//				start_lifted = end_lifted;
//				end_lifted = tmp;
//			}

//			BedLine out_bed;
//			try
//			{
//				out_bed = liftOver(in.chr(), in.start(), in.end(), hg19_to_hg38);
//			}
//			catch (ArgumentException e)
//			{
//				std::cout << "Only server threw error.... " << e.message().toStdString() << "\n";
//				std::cout << "mine lifted: " << start_lifted.toString().toStdString() << " - " << end_lifted.toString().toStdString() <<"\n";
//			}
//			QByteArray out_chr = out_bed.chr().str();
//			int out_start = out_bed.start();
//			int out_end = out_bed.end();

//			//std::cout << "line: " << line << "\t" << chr.toStdString() << ":" << start << "-" << end << "\tlifted to: " << start_lifted.chr.toStdString() << ":" << start_lifted.pos << "-" << end_lifted.pos << "\n";

//			if (QString(start_lifted.chr) != QString(out_chr) || start_lifted.pos != out_start || end_lifted.pos != out_end)
//			{
//				wrong++;
//				std::cout << "not the same coordinates!\n";
//				continue;
//			}
//			correct++;
//			S_EQUAL(QString(start_lifted.chr), QString(out_chr));
//			I_EQUAL(start_lifted.pos, out_start);
//			I_EQUAL(end_lifted.pos, out_end);
//		}
//		std::cout << "Positions wrongly mapped: " << wrong << "\n";
//	}

//private:
//	BedLine liftOver(const Chromosome& chr, int start, int end, bool hg19_to_hg38)
//	{
//		//special handling of chrMT (they are the same for GRCh37 and GRCh38)
//		if (chr.strNormalized(true)=="chrMT") return BedLine(chr, start, end);

//		//convert start to BED format (0-based)
//		start -= 1;

//		//call lift-over webservice
//		QString url = "https://portal.img.med.uni-tuebingen.de/LiftOver/liftover.php?chr=" + chr.strNormalized(true) + "&start=" + QString::number(start) + "&end=" + QString::number(end);
//		if (!hg19_to_hg38) url += "&dir=hg38_hg19";
//		QString output = HttpRequestHandler(HttpRequestHandler::ProxyType::NONE).get(url);

//		//handle error from webservice
//		if (output.contains("ERROR")) THROW(ArgumentException, "genomic coordinate lift-over failed: " + output);

//		//convert output to region
//		BedLine region = BedLine::fromString(output);
//		if (!region.isValid()) THROW(ArgumentException, "genomic coordinate lift-over failed: Could not convert output '" + output + "' to valid region");

//		//revert to 1-based
//		region.setStart(region.start()+1);

//		return region;
//	}




};
