#ifndef TESTFRAMEWORK_NGS_H
#define TESTFRAMEWORK_NGS_H

#include "TestFramework.h"
#include "FastqFileStream.h"

namespace TFW
{
	inline void fastqStatistics(QString fastq, QString out)
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
			int length = e.bases.count();
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
			QFAIL(("Cannot open FASTQ statistics output file " + out).toLatin1());
		}

		output.write(("Reads: " + QString::number(reads) + "\n").toLatin1());
		foreach(int length, read_len.keys())
		{
			output.write((QString::number(length) + " base reads: " + QString::number(read_len[length]) + "\n").toLatin1());
		}
		output.write("\n");

		output.write(("Bases: " + QString::number(bases) + "\n").toLatin1());
		foreach(char base, base_count.keys())
		{
			output.write((QString(base) + ": " + QString::number(base_count[base]) + "\n").toLatin1());
		}

		output.close();
	}

	inline void fastqCheckPair(QString in1, QString in2)
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
					QFAIL(("Paired-end FASTQ headers differ! F: " + h1 + " R: " + h2 + " IN1: " + in1 + " IN2: " + in2).toLatin1());
				}
			}
		}

		if(!stream1.atEnd() || !stream2.atEnd())
		{
			QFAIL(("Paired-end FASTQ files differ in length! IN1: " + in1 + " IN2: " + in2).toLatin1());
		}
	}

}

#endif 
