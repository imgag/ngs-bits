#include "../TestFramework.h"
#include "FastqFileStream.h"
#include "Helper.h"

class FastqFileStream_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void entry_trimQuality()
	{
		//empty sequence trimming
		FastqEntry e;
		e.header = "bla";
		e.header2 = "bla2";
		int result = e.trimQuality(15);
		QCOMPARE(result, 0);
		QCOMPARE(e.bases, QByteArray(""));
		QCOMPARE(e.qualities, QByteArray(""));

		//sequence of half window size - no trimming
		e.bases = "ACG";
		e.qualities = "###";
		result = e.trimQuality(15);
		QCOMPARE(result, 0);
		QCOMPARE(e.bases, QByteArray("ACG"));
		QCOMPARE(e.qualities, QByteArray("###"));

		//sequence of window size - no trimming
		e.bases = "ACGTA";
		e.qualities = "IIIII";
		result = e.trimQuality(15);
		QCOMPARE(result, 0);
		QCOMPARE(e.bases, QByteArray("ACGTA"));
		QCOMPARE(e.qualities, QByteArray("IIIII"));

		//sequence of window size - trimming
		e.bases = "ACGTA";
		e.qualities = "#####";
		result = e.trimQuality(15);
		QCOMPARE(result, 5);
		QCOMPARE(e.bases, QByteArray(""));
		QCOMPARE(e.qualities, QByteArray(""));

		//sequence longer than window size - no trimming
		e.bases = "ACGTACGTACGTACGTACGTACGTACGTACGT";
		e.qualities = "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII";
		result = e.trimQuality(15);
		QCOMPARE(result, 0);
		QCOMPARE(e.bases, QByteArray("ACGTACGTACGTACGTACGTACGTACGTACGT"));
		QCOMPARE(e.qualities, QByteArray("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"));

		//sequence longer than window size - trimming
		e.bases = "ACGTACGTACGTACGTACGTACGTACGTACGT";
		e.qualities = "IIIIIIIIIIIIIIIIIIIIIIIIIII#####";
		result = e.trimQuality(15);
		QCOMPARE(result, 5);
		QCOMPARE(e.bases, QByteArray("ACGTACGTACGTACGTACGTACGTACG"));
		QCOMPARE(e.qualities, QByteArray("IIIIIIIIIIIIIIIIIIIIIIIIIII"));

		//sequence longer than window size - trimming
		e.bases = "ACGTACGTACGTACGTACGTACGTACGTACGT";
		e.qualities = "?????????????????????:50+#######";
		result = e.trimQuality(15);
		QCOMPARE(result, 8);
		QCOMPARE(e.bases, QByteArray("ACGTACGTACGTACGTACGTACGT"));
		QCOMPARE(e.qualities, QByteArray("?????????????????????:50"));
	}


	void entry_trimN()
	{
		//empty sequence trimming
		FastqEntry e;
		e.header = "bla";
		e.header2 = "bla2";
		int result = e.trimN(7);
		QCOMPARE(result, 0);
		QCOMPARE(e.bases, QByteArray(""));
		QCOMPARE(e.qualities, QByteArray(""));

		//sequence of half window size - no trimming
		e.bases = "ACG";
		e.qualities = "###";
		result = e.trimN(7);
		QCOMPARE(result, 0);
		QCOMPARE(e.bases, QByteArray("ACG"));
		QCOMPARE(e.qualities, QByteArray("###"));

		//no trimming
		e.bases = "ACGTANNNNNN";
		e.qualities = "IIIIIABCDEF";
		result = e.trimN(7);
		QCOMPARE(result, 0);
		QCOMPARE(e.bases, QByteArray("ACGTANNNNNN"));
		QCOMPARE(e.qualities, QByteArray("IIIIIABCDEF"));

		//trimming end
		e.bases = "ACGTANNNNNNN";
		e.qualities = "IIIIIABCDEFG";
		result = e.trimN(7);
		QCOMPARE(result, 7);
		QCOMPARE(e.bases, QByteArray("ACGTA"));
		QCOMPARE(e.qualities, QByteArray("IIIII"));

		//trimming end
		e.bases = "ACGTANNNNNNANNNNNNN";
		e.qualities = "IIIIIABCDEFGABCDEFG";
		result = e.trimN(7);
		QCOMPARE(result, 7);
		QCOMPARE(e.bases, QByteArray("ACGTANNNNNNA"));
		QCOMPARE(e.qualities, QByteArray("IIIIIABCDEFG"));

		//trimming start
		e.bases = "NNNNNNNACGTANNNNNNA";
		e.qualities = "IIIIIABCDEFGABCDEFG";
		result = e.trimN(7);
		QCOMPARE(result, 19);
		QCOMPARE(e.bases, QByteArray(""));
		QCOMPARE(e.qualities, QByteArray(""));

		//trimming middle
		e.bases = "ACGTANNNNNNNNNNNNNN";
		e.qualities = "IIIIIABCDEAFGABCDEFG";
		result = e.trimN(7);
		QCOMPARE(result, 14);
		QCOMPARE(e.bases, QByteArray("ACGTA"));
		QCOMPARE(e.qualities, QByteArray("IIIII"));
	}

	void read_gzipped()
	{
		FastqFileStream stream(QFINDTESTDATA("data_in/example1.fastq.gz"));

		QVERIFY(!stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		QCOMPARE(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		QVERIFY(stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray(""));
	}

	void read_plain()
	{
		FastqFileStream stream(QFINDTESTDATA("data_in/example2.fastq"));

		QVERIFY(!stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		QCOMPARE(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		QVERIFY(stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray(""));
	}

	void read_plain_emptylineatend()
	{
		FastqFileStream stream(QFINDTESTDATA("data_in/example3.fastq"));

		QVERIFY(!stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		QCOMPARE(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		QVERIFY(stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray(""));
	}

	void read_plain_empty()
	{
		FastqFileStream stream(QFINDTESTDATA("data_in/example4.fastq"));
		//This cannot be tested because Windows/Linux behave differently here. Under Linux the stream is atEnd after opening...
		//QVERIFY(!stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray(""));
		QCOMPARE(entry.bases, QByteArray(""));
		QCOMPARE(entry.header2, QByteArray(""));
		QCOMPARE(entry.qualities, QByteArray(""));

		QVERIFY(stream.atEnd());
	}

	void read_plain_crlf()
	{
		FastqFileStream stream(QFINDTESTDATA("data_in/example5.fastq"));

		QVERIFY(!stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		QCOMPARE(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		QVERIFY(stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray(""));
	}

	void write_gzipped()
	{
		//copy Fastq data to temporary file
		QString tmp_file = Helper::tempFileName(".fastq.gz");
		FastqOutfileStream out(tmp_file, false);
		{
			FastqFileStream stream(QFINDTESTDATA("data_in/example1.fastq.gz"));
			while(!stream.atEnd())
			{
				FastqEntry entry;
				stream.readEntry(entry);
				out.write(entry);
			}
		}
		out.close();


		//check that the data is correctly written
		FastqFileStream stream(tmp_file);

		QVERIFY(!stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		QCOMPARE(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		QCOMPARE(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		QVERIFY(!stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		QVERIFY(stream.atEnd());
		stream.readEntry(entry);
		QCOMPARE(entry.header, QByteArray(""));

		//clean up
		QFile::remove(tmp_file);
	}

};

TFW_DECLARE(FastqFileStream_Test)

