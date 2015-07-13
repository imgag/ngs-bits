#include "TestFramework.h"
#include "FastqFileStream.h"
#include "Helper.h"

TEST_CLASS(FastqFileStream_Test)
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
		I_EQUAL(result, 0);
		S_EQUAL(e.bases, QByteArray(""));
		S_EQUAL(e.qualities, QByteArray(""));

		//sequence of half window size - no trimming
		e.bases = "ACG";
		e.qualities = "###";
		result = e.trimQuality(15);
		I_EQUAL(result, 0);
		S_EQUAL(e.bases, QByteArray("ACG"));
		S_EQUAL(e.qualities, QByteArray("###"));

		//sequence of window size - no trimming
		e.bases = "ACGTA";
		e.qualities = "IIIII";
		result = e.trimQuality(15);
		I_EQUAL(result, 0);
		S_EQUAL(e.bases, QByteArray("ACGTA"));
		S_EQUAL(e.qualities, QByteArray("IIIII"));

		//sequence of window size - trimming
		e.bases = "ACGTA";
		e.qualities = "#####";
		result = e.trimQuality(15);
		I_EQUAL(result, 5);
		S_EQUAL(e.bases, QByteArray(""));
		S_EQUAL(e.qualities, QByteArray(""));

		//sequence longer than window size - no trimming
		e.bases = "ACGTACGTACGTACGTACGTACGTACGTACGT";
		e.qualities = "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII";
		result = e.trimQuality(15);
		I_EQUAL(result, 0);
		S_EQUAL(e.bases, QByteArray("ACGTACGTACGTACGTACGTACGTACGTACGT"));
		S_EQUAL(e.qualities, QByteArray("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"));

		//sequence longer than window size - trimming
		e.bases = "ACGTACGTACGTACGTACGTACGTACGTACGT";
		e.qualities = "IIIIIIIIIIIIIIIIIIIIIIIIIII#####";
		result = e.trimQuality(15);
		I_EQUAL(result, 5);
		S_EQUAL(e.bases, QByteArray("ACGTACGTACGTACGTACGTACGTACG"));
		S_EQUAL(e.qualities, QByteArray("IIIIIIIIIIIIIIIIIIIIIIIIIII"));

		//sequence longer than window size - trimming
		e.bases = "ACGTACGTACGTACGTACGTACGTACGTACGT";
		e.qualities = "?????????????????????:50+#######";
		result = e.trimQuality(15);
		I_EQUAL(result, 8);
		S_EQUAL(e.bases, QByteArray("ACGTACGTACGTACGTACGTACGT"));
		S_EQUAL(e.qualities, QByteArray("?????????????????????:50"));
	}


	void entry_trimN()
	{
		//empty sequence trimming
		FastqEntry e;
		e.header = "bla";
		e.header2 = "bla2";
		int result = e.trimN(7);
		I_EQUAL(result, 0);
		S_EQUAL(e.bases, QByteArray(""));
		S_EQUAL(e.qualities, QByteArray(""));

		//sequence of half window size - no trimming
		e.bases = "ACG";
		e.qualities = "###";
		result = e.trimN(7);
		I_EQUAL(result, 0);
		S_EQUAL(e.bases, QByteArray("ACG"));
		S_EQUAL(e.qualities, QByteArray("###"));

		//no trimming
		e.bases = "ACGTANNNNNN";
		e.qualities = "IIIIIABCDEF";
		result = e.trimN(7);
		I_EQUAL(result, 0);
		S_EQUAL(e.bases, QByteArray("ACGTANNNNNN"));
		S_EQUAL(e.qualities, QByteArray("IIIIIABCDEF"));

		//trimming end
		e.bases = "ACGTANNNNNNN";
		e.qualities = "IIIIIABCDEFG";
		result = e.trimN(7);
		I_EQUAL(result, 7);
		S_EQUAL(e.bases, QByteArray("ACGTA"));
		S_EQUAL(e.qualities, QByteArray("IIIII"));

		//trimming end
		e.bases = "ACGTANNNNNNANNNNNNN";
		e.qualities = "IIIIIABCDEFGABCDEFG";
		result = e.trimN(7);
		I_EQUAL(result, 7);
		S_EQUAL(e.bases, QByteArray("ACGTANNNNNNA"));
		S_EQUAL(e.qualities, QByteArray("IIIIIABCDEFG"));

		//trimming start
		e.bases = "NNNNNNNACGTANNNNNNA";
		e.qualities = "IIIIIABCDEFGABCDEFG";
		result = e.trimN(7);
		I_EQUAL(result, 19);
		S_EQUAL(e.bases, QByteArray(""));
		S_EQUAL(e.qualities, QByteArray(""));

		//trimming middle
		e.bases = "ACGTANNNNNNNNNNNNNN";
		e.qualities = "IIIIIABCDEAFGABCDEFG";
		result = e.trimN(7);
		I_EQUAL(result, 14);
		S_EQUAL(e.bases, QByteArray("ACGTA"));
		S_EQUAL(e.qualities, QByteArray("IIIII"));
	}

	void read_gzipped()
	{
		FastqFileStream stream(TESTDATA("data_in/example1.fastq.gz"));

		IS_FALSE(stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		S_EQUAL(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		IS_TRUE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray(""));
	}

	void read_plain()
	{
		FastqFileStream stream(TESTDATA("data_in/example2.fastq"));

		IS_FALSE(stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		S_EQUAL(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		IS_TRUE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray(""));
	}

	void read_plain_emptylineatend()
	{
		FastqFileStream stream(TESTDATA("data_in/example3.fastq"));

		IS_FALSE(stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		S_EQUAL(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		IS_TRUE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray(""));
	}

	void read_plain_empty()
	{
		FastqFileStream stream(TESTDATA("data_in/example4.fastq"));
		//This cannot be tested because Windows/Linux behave differently here. Under Linux the stream is atEnd after opening...
		//IS_FALSE(stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray(""));
		S_EQUAL(entry.bases, QByteArray(""));
		S_EQUAL(entry.header2, QByteArray(""));
		S_EQUAL(entry.qualities, QByteArray(""));

		IS_TRUE(stream.atEnd());
	}

	void read_plain_crlf()
	{
		FastqFileStream stream(TESTDATA("data_in/example5.fastq"));

		IS_FALSE(stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		S_EQUAL(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		IS_TRUE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray(""));
	}

	void write_gzipped()
	{
		//copy Fastq data to temporary file
		QString tmp_file = Helper::tempFileName(".fastq.gz");
		FastqOutfileStream out(tmp_file, false);
		{
			FastqFileStream stream(TESTDATA("data_in/example1.fastq.gz"));
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

		IS_FALSE(stream.atEnd());
		FastqEntry entry;
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.bases, QByteArray("NACTCCGGTGTCGGTCTCGTAGGCCATTTTAGAAGCGAATAAATCGATGNATTCGANCNCNNNNNNNNATCGNNAGAGCTCGTANGCCGTCTTCTGCTTGANNNNNNN"));
		S_EQUAL(entry.header2, QByteArray("+NG-5232_4_1_1022_17823#0/1"));
		S_EQUAL(entry.qualities, QByteArray("#'''')(++)AAAAAAAAAA########################################################################################"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1025_18503#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1026_21154#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1028_9044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_3041#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_18565#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1031_20044#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1032_18092#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_5386#0/1"));

		IS_FALSE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray("@NG-5232_4_1_1033_2620#0/1"));

		IS_TRUE(stream.atEnd());
		stream.readEntry(entry);
		S_EQUAL(entry.header, QByteArray(""));

		//clean up
		QFile::remove(tmp_file);
	}

};
