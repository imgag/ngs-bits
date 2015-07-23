#ifndef AUXILARY_H
#define AUXILARY_H

#include "FastqFileStream.h"
#include <QThreadPool>
#include <Pileup.h>
#include "StatisticsReads.h"

///Output stream datastructure
struct TrimmingData
{
	TrimmingData()
	: analysis_pool()
	, reads_queued(0)
	, out1_out2_mutex()
	, out1(0)
	, out2(0)
	, out3(0)
	, out4(0)
	{
	}

	//analysis datastructures
	QThreadPool analysis_pool;
	int reads_queued;

	//output streams
	QMutex out1_out2_mutex; ///< Mutex to ensure the order of reads in forward/reverse read file matches
	FastqOutfileStream* out1;
	FastqOutfileStream* out2;
	FastqOutfileStream* out3;
	FastqOutfileStream* out4;

	///Wait until analysis is finished and close output streams (to flush data).
	void closeOutStreams()
	{
		analysis_pool.waitForDone();

		out1->close();
		out2->close();
		if (out3) out3->close();
		if (out4) out4->close();
	}
};

///Input parameters datastructure.
struct TrimmingParameters
{
	TrimmingParameters()
	: adapter_overlap(10)
	, max_reads_queued(1000)
	{
	}

	int adapter_overlap;
	int max_reads_queued;
	QByteArray a1;
	QByteArray a2;
	int a_size;
	double match_perc;
	double mep;
	int min_len_s;
	int min_len_p;
	int qcut;
	int qwin;
	int qoff;
	int ncut;
	bool debug;
	QString qc;
};

///Statistics datastructure.
struct TrimmingStatistics
{
	TrimmingStatistics()
	: mutex()
	, read_num(0)
	, bases_remaining()
	, acons1()
	, acons2()
	, reads_trimmed_insert(0.0)
	, reads_trimmed_adapter(0.0)
	, reads_trimmed_q(0.0)
	, reads_trimmed_n(0.0)
	, reads_removed(0.0)
	, bases_perc_trim_sum(0.0)
	, qc()
	{
		//reserve/resize array - reallocation of the array content can cause a crash because the array is accessed in parallel by several threads
		bases_remaining.reserve(1000);
		acons1.resize(40);
		acons2.resize(40);
	}

	QMutex mutex; //Mutex for thread-safe writing
	int read_num;
	QVector<double> bases_remaining;
	QVector<Pileup> acons1;
	QVector<Pileup> acons2;
	double reads_trimmed_insert;
	double reads_trimmed_adapter;
	double reads_trimmed_q;
	double reads_trimmed_n;
	double reads_removed;
	double bases_perc_trim_sum;
	StatisticsReads qc;

	void writeStatistics(QTextStream& out, const TrimmingParameters& params_)
	{
		out << "Reads (forward + reverse): " << read_num << endl;
		out << endl;
		out << "Reads trimmed by insert match: " << (long)reads_trimmed_insert << endl;
		out << "Reads trimmed by adapter match: " << (long)reads_trimmed_adapter << endl;
		out << "Reads trimmed by quality: " << (long)reads_trimmed_q << endl;
		out << "Reads trimmed by N stretches: " << (long)reads_trimmed_n << endl;
		double reads_trimmed = reads_trimmed_insert + reads_trimmed_adapter;
		out << "Trimmed reads: " << (long)reads_trimmed << " of " << read_num << " (" << QString::number(100.0*reads_trimmed/read_num, 'f', 2) << "%)" << endl;
		out << "Removed reads: " << (long)reads_removed << " of " << read_num << " (" << QString::number(100.0*reads_removed/read_num, 'f', 2) << "%)" << endl;
		out << "Removed bases: " << QString::number(100.0*bases_perc_trim_sum/read_num, 'f', 2) << "%" << endl;
		out << endl;

		//print consensus adapter sequence
		QByteArray acons1_seq;
		for (int i=0; i<acons1.count(); ++i)
		{
			int depth = acons1[i].depth(false);
			if (depth<20) break;
			int max = acons1[i].max();
			if ((double)max/depth<=0.5) acons1_seq.append('N');
			else if (acons1[i].a()==max) acons1_seq.append('A');
			else if (acons1[i].c()==max) acons1_seq.append('C');
			else if (acons1[i].g()==max) acons1_seq.append('G');
			else if (acons1[i].t()==max) acons1_seq.append('T');
			if (i==39) break;
		}
		out << "Forward adapter sequence (given)    : " << params_.a1 << endl;
		out << "Forward adapter sequence (consensus): " << acons1_seq << endl;
		QByteArray acons2_seq;
		for (int i=0; i<acons2.count(); ++i)
		{
			int depth = acons2[i].depth(false);
			if (depth<20) break;
			int max = acons2[i].max();
			if ((double)max/depth<=0.5) acons2_seq.append('N');
			else if (acons2[i].a()==max) acons2_seq.append('A');
			else if (acons2[i].c()==max) acons2_seq.append('C');
			else if (acons2[i].g()==max) acons2_seq.append('G');
			else if (acons2[i].t()==max) acons2_seq.append('T');
			if (i==39) break;
		}
		out << "Reverse adapter sequence (given)    : " << params_.a2 << endl;
		out << "Reverse adapter sequence (consensus): " << acons2_seq << endl;
		out << endl;

		//print length distribution after trimming
		out << "Read length distribution after trimming:" << endl;
		for (int i=0; i<bases_remaining.count(); ++i)
		{
			out << QString::number(i).rightJustified(4, ' ') << ": " << (long)(bases_remaining[i]) << endl;
		}
	}
};


#endif // AUXILARY_H

