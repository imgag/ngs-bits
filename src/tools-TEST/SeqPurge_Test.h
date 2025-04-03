#include "TestFramework.h"
#include "FastqFileStream.h"
#include <QTime>

void fastqStatistics(QString fastq, QString out)
{
	//init
	double reads = 0.0;
	QMap<int, int> read_len;
	double bases = 0.0;
	QMap<char, int> base_count;
	base_count['A'] = 0;
	base_count['C'] = 0;
	base_count['G'] = 0;
	base_count['N'] = 0;
	base_count['T'] = 0;

	//calculate statistics
	FastqFileStream stream(fastq, true);
	while(!stream.atEnd())
	{
		FastqEntry e;
		stream.readEntry(e);

		//read data
        int length = e.bases.size();
		reads += 1;
		if (!read_len.contains(length)) read_len.insert(length, 0);
		read_len[length] += 1;

		//base data
		bases += length;
		foreach(char base, e.bases)
		{
			base_count[base] += 1;
		}
	}

	//output
	QFile output(out);
	if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		THROW(FileAccessException, "Cannot open FASTQ statistics output file '" + out + "'");
	}

	output.write(("Reads: " + QString::number(reads) + "\n").toUtf8());
	foreach(int length, read_len.keys())
	{
		output.write((QString::number(length) + " base reads: " + QString::number(read_len[length]) + "\n").toUtf8());
	}
	output.write("\n");

	output.write(("Bases: " + QString::number(bases) + "\n").toUtf8());
	foreach(char base, base_count.keys())
	{
		output.write((QString(base) + ": " + QString::number(base_count[base]) + "\n").toUtf8());
	}

	output.close();
}

QString fastqCheckPair(QString in1, QString in2)
{
	FastqFileStream stream1(in1, true);
	FastqFileStream stream2(in2, true);
	while(!stream1.atEnd() && !stream2.atEnd())
	{
		FastqEntry e1;
		stream1.readEntry(e1);

		FastqEntry e2;
		stream2.readEntry(e2);

		QByteArray h1 = e1.header;
		QByteArray h2 = e2.header;
		if (h1!=h2)
		{
			h1 = h1.split(' ').at(0);
			h2 = h2.split(' ').at(0);
			if (h1!=h2)
			{
				return "Paired-end FASTQ headers differ!\nF: " + h1 + "\nR: " + h2 + "\nIN1: " + in1 + "\nIN2: " + in2;
			}
		}
	}

	if(!stream1.atEnd() || !stream2.atEnd())
	{
		return "Paired-end FASTQ files differ in length!\nIN1: " + in1 + "\nIN2: " + in2;
	}

	return "";
}


TEST_CLASS(SeqPurge_Test)
{
Q_OBJECT
private slots:

	//MiSeq 151 cycles - hpHSPv2 panel - with qc
	void test_01()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 out/SeqPurge_out1.fastq.gz -out2 out/SeqPurge_out2.fastq.gz -ncut 0 -qcut 0 -min_len 15 -qc out/SeqPurge_out1.qcML -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out1.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out2.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out1.fastq.gz", TESTDATA("data_out/SeqPurge_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out2.fastq.gz", TESTDATA("data_out/SeqPurge_out2.fastq.gz"));

		//qc comparison
        REMOVE_LINES("out/SeqPurge_out1.qcML", QRegularExpression("creation "));
        REMOVE_LINES("out/SeqPurge_out1.qcML", QRegularExpression("source file"));
        REMOVE_LINES("out/SeqPurge_out1.qcML", QRegularExpression("<binary>"));
		COMPARE_FILES("out/SeqPurge_out1.qcML", TESTDATA("data_out/SeqPurge_out1.qcML"));
	}
	
	//MiSeq 151 cycles - test data where homopolymers mess up one read direction
	void test_02()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in3.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in4.fastq.gz") + " -out1 out/SeqPurge_out3.fastq.gz -out2 out/SeqPurge_out4.fastq.gz -ncut 0 -qcut 0 -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out3.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out4.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out3.fastq.gz", TESTDATA("data_out/SeqPurge_out3.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out4.fastq.gz", TESTDATA("data_out/SeqPurge_out4.fastq.gz"));
	}
	
	//HiSeq 101 cycles - test data where the adapter sequences are not obvious for sequence hits
	void test_03()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in5.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in6.fastq.gz") + " -out1 out/SeqPurge_out5.fastq.gz -out2 out/SeqPurge_out6.fastq.gz -ncut 0 -qcut 0 -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out5.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out6.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out5.fastq.gz", TESTDATA("data_out/SeqPurge_out5.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out6.fastq.gz", TESTDATA("data_out/SeqPurge_out6.fastq.gz"));
	}
	
	//HiSeq 151 cycles - test data where reads have different lengths due to trimming by Illumina
	void test_04()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in7.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in8.fastq.gz") + " -out1 out/SeqPurge_out7.fastq.gz -out2 out/SeqPurge_out8.fastq.gz -a1 CTGTCTCTTATACACATCT -a2 CTGTCTCTTATACACATCT -ncut 0 -qcut 0 -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out7.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out8.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out7.fastq.gz", TESTDATA("data_out/SeqPurge_out7.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out8.fastq.gz", TESTDATA("data_out/SeqPurge_out8.fastq.gz"));
	}
	
	//MiSeq 151 cycles - with quality trimming
	void test_05()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 out/SeqPurge_out9.fastq.gz -out2 out/SeqPurge_out10.fastq.gz -qcut 15 -ncut 0 -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out9.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out10.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out9.fastq.gz", TESTDATA("data_out/SeqPurge_out9.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out10.fastq.gz", TESTDATA("data_out/SeqPurge_out10.fastq.gz"));
	}
	
	//MiSeq 151 cycles - with N trimming
	void test_06()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 out/SeqPurge_out11.fastq.gz -out2 out/SeqPurge_out12.fastq.gz -ncut 7 -qcut 0 -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out11.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out12.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out11.fastq.gz", TESTDATA("data_out/SeqPurge_out11.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out12.fastq.gz", TESTDATA("data_out/SeqPurge_out12.fastq.gz"));
	}

	//MiSeq 151 cycles - with quality trimming and N trimming - with singleton output
	void test_07()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 out/SeqPurge_out13.fastq.gz -out2 out/SeqPurge_out14.fastq.gz -out3 out/SeqPurge_out15 -qcut 25 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out13.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out14.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out13.fastq.gz", TESTDATA("data_out/SeqPurge_out13.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out14.fastq.gz", TESTDATA("data_out/SeqPurge_out14.fastq.gz"));
	
		IS_TRUE(QFile::exists("out/SeqPurge_out15_R1.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out15_R2.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out15_R1.fastq.gz", TESTDATA("data_out/SeqPurge_out15_R1.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out15_R2.fastq.gz", TESTDATA("data_out/SeqPurge_out15_R2.fastq.gz"));
	}

	//MiSeq 300 cycles - test with very long reads (factorial overflow bug)
	void test_08()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in9.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in10.fastq.gz") + " -out1 out/SeqPurge_out16.fastq.gz -out2 out/SeqPurge_out17.fastq.gz -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out16.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out17.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out16.fastq.gz", TESTDATA("data_out/SeqPurge_out16.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out17.fastq.gz", TESTDATA("data_out/SeqPurge_out17.fastq.gz"));
	}

	//GAIIx 76/77cycles - different read length bug
	void test_09()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in11.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in12.fastq.gz") + " -out1 out/SeqPurge_out18.fastq.gz -out2 out/SeqPurge_out19.fastq.gz -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out18.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out19.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out18.fastq.gz", TESTDATA("data_out/SeqPurge_out18.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out19.fastq.gz", TESTDATA("data_out/SeqPurge_out19.fastq.gz"));
	}

	//MiSeq 151 cycles - with error correction
	void test_10()
	{
		EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 out/SeqPurge_out20.fastq.gz -out2 out/SeqPurge_out21.fastq.gz -ncut 0 -qcut 0 -ec -min_len 15 -block_size 100 -block_prefetch 1");
		IS_TRUE(QFile::exists("out/SeqPurge_out20.fastq.gz"));
		IS_TRUE(QFile::exists("out/SeqPurge_out21.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out20.fastq.gz", TESTDATA("data_out/SeqPurge_out20.fastq.gz"));
		COMPARE_GZ_FILES("out/SeqPurge_out21.fastq.gz", TESTDATA("data_out/SeqPurge_out21.fastq.gz"));
	}

	//multi-thread test
	void test_multithread()
	{
        QElapsedTimer timer;
		for (int i=1; i<=8; ++i)
		{
			QString suffix = QString::number(i) + "threads";
			QString out1 = "out/SeqPurge_"+suffix+"_R1.fastq.gz";
			QString out2 = "out/SeqPurge_"+suffix+"_R2.fastq.gz";
			timer.restart();
			EXECUTE("SeqPurge", "-in1 " + TESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + TESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 "+out1+" -out2 "+out2+" -ncut 0 -qcut 0 -min_len 15 -qc out/SeqPurge_"+suffix+".qcML -summary out/SeqPurge_"+suffix+".log -block_size 100 -threads " + QString::number(i));

			//compare fastq statistics
			QString out1_stats = "out/SeqPurge_"+suffix+"_R1.stats";
			fastqStatistics(out1, out1_stats);
			COMPARE_FILES(out1_stats, "out/SeqPurge_1threads_R1.stats");
			QString out2_stats = "out/SeqPurge_"+suffix+"_R2.stats";
			fastqStatistics(out2, out2_stats);
			COMPARE_FILES(out2_stats, "out/SeqPurge_1threads_R2.stats");

			//check that reads are correctly paired in forward/reverse file
			S_EQUAL(fastqCheckPair(out1, out2), "");

			//qc comparison
            REMOVE_LINES("out/SeqPurge_"+suffix+".qcML", QRegularExpression("creation "));
            REMOVE_LINES("out/SeqPurge_"+suffix+".qcML", QRegularExpression("source file"));
            REMOVE_LINES("out/SeqPurge_"+suffix+".qcML", QRegularExpression("<binary>"));
			COMPARE_FILES("out/SeqPurge_"+suffix+".qcML", "out/SeqPurge_1threads.qcML");
		}
	}
};


