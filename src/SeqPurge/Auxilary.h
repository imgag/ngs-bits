#ifndef AUXILARY_H
#define AUXILARY_H

#include <Pileup.h>
#include <QFile>
#include <QThreadPool>
#include <QMutex>
#include "FastqFileStream.h"
#include "StatisticsReads.h"


const int MAXLEN = 1000;

///Analysis status
enum AnalysisStatus
{
	TO_BE_ANALYZED,
	TO_BE_WRITTEN,
	DONE
};

///Analysis data for worker.
struct AnalysisJob
{
	//constructor
	AnalysisJob() = delete;
	AnalysisJob(int i, int block_size)
	{
		clear();

		index = i;

		r1.resize(block_size);
		r2.resize(block_size);
		length_r1_orig.resize(block_size);
		length_r2_orig.resize(block_size);
	}

	int index; //index used to identify this job in signal/slots

	QVector<FastqEntry> r1;
	QVector<FastqEntry> r2;
	int read_count; //number of reads to process (normally 'block_size', but not for for last job)

	AnalysisStatus status; //only used to write statistics in progress output

	//statistics stuff
	QVector<int> length_r1_orig;
	QVector<int> length_r2_orig;
	int reads_trimmed_insert;
	int reads_trimmed_adapter;
	int reads_trimmed_q;
	int reads_trimmed_n;

	void clear()
	{
		//note: index, r1, r2, length_r1_orig, length_r2_orig must not be cleared
		read_count = -1;
		status = DONE;

		reads_trimmed_insert = 0;
		reads_trimmed_adapter = 0;
		reads_trimmed_q = 0;
		reads_trimmed_n = 0;
	}
};

//Input stream data
struct InputStreams
{
	int current_index = 0;
	QSharedPointer<FastqFileStream> istream1;
	QSharedPointer<FastqFileStream> istream2;
};

//Output stream data
struct OutputStreams
{
	//summary stream
	QSharedPointer<QFile> summary_file;
	QSharedPointer<QTextStream> summary_stream;

	//FASTQ output steams
	QSharedPointer<FastqOutfileStream> ostream1;
	QSharedPointer<FastqOutfileStream> ostream2;
	QSharedPointer<FastqOutfileStream> ostream3;
	QSharedPointer<FastqOutfileStream> ostream4;

	//parallel writing of osteam1 and ostream2

	bool ostream1_done;
	bool ostream2_done;
	QThreadPool ostream1_thread;
	QThreadPool ostream2_thread;
	QString ostream1_error;
	QString ostream2_error;
};

///Input parameters datastructure.
struct TrimmingParameters
{
	TrimmingParameters()
	: adapter_overlap(10)
	{
	}

	QStringList files_in1;
	QStringList files_in2;
	QString out1;
	QString out2;
	QString out3;
	QString summary;
	int adapter_overlap;
	long max_reads_queued;
	QByteArray a1;
	QByteArray a2;
	int a_size;
	double match_perc;
	double mep;
	int min_len;
	int block_prefetch;
	int block_size;
	int threads;
	int progress;
	int qcut;
	int qwin;
	int qoff;
	int ncut;
	bool ec;
	bool debug;
	int compression_level;
	QString qc;
};

///Statistics datastructure.
struct TrimmingStatistics
{
	TrimmingStatistics()
	: read_num(0)
	, bases_remaining(MAXLEN, 0) //fixed size - reallocation prevents parallel access
	, acons1(40)
	, acons2(40)
	, reads_trimmed_insert(0.0)
	, reads_trimmed_adapter(0.0)
	, reads_trimmed_q(0.0)
	, reads_trimmed_n(0.0)
	, reads_removed(0.0)
	, bases_perc_trim_sum(0.0)
	, qc()
	{
	}

	long read_num;
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
	QMutex qc_mutex;

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
		int max = bases_remaining.count()-1;
		while(bases_remaining[max]==0) max -= 1;
		for (int i=0; i<=max; ++i)
		{
			out << QString::number(i).rightJustified(4, ' ') << ": " << (long)(bases_remaining[i]) << endl;
		}
	}
};

///Statistics datastructure.
struct ErrorCorrectionStatistics
{
	ErrorCorrectionStatistics()
		: mismatch_r1(MAXLEN, 0) //fixed size - reallocation prevents parallel access
		, mismatch_r2(MAXLEN, 0) //fixed size - reallocation prevents parallel access
		, errors_per_read(MAXLEN, 0) //fixed size - reallocation prevents parallel access
	{
	}

	QVector<long> mismatch_r1;
	QVector<long> mismatch_r2;
	QVector<long> errors_per_read;

	void writeStatistics(QTextStream& out)
	{
		//print read error per cycle (read 1)
		out << endl;
		out << "Read error per cycle (read 1):" << endl;
		int max = mismatch_r1.count()-1;
		while(mismatch_r1[max]==0) max -= 1;
		for (int i=1; i<=max; ++i)
		{
			out << QString::number(i).rightJustified(4, ' ') << ": " << mismatch_r1[i] << endl;
		}

		//print read error per cycle (read 2)
		out << endl;
		out << "Read error per cycle (read 2):" << endl;
		max = mismatch_r2.count()-1;
		while(mismatch_r2[max]==0) max -= 1;
		for (int i=1; i<=max; ++i)
		{
			out << QString::number(i).rightJustified(4, ' ') << ": " << mismatch_r2[i] << endl;
		}

		//print error count distribution
		out << endl;
		out << "Read error count distribution:" << endl;
		max = errors_per_read.count()-1;
		while(errors_per_read[max]==0) max -= 1;
		for (int i=1; i<=max; ++i)
		{
			out << QString::number(i).rightJustified(4, ' ') << ": " << errors_per_read[i] << endl;
		}
	}

};

#endif // AUXILARY_H

