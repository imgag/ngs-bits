#ifndef STATISTICSREADS_H
#define STATISTICSREADS_H

#include "cppNGS_global.h"
#include <QSet>
#include "FastqFileStream.h"
#include "QCCollection.h"
#include "Pileup.h"
#include <QMap>
#include <QMutex>
#include "BamReader.h"
#include "Histogram.h"

///Read statistics for quality control.
class CPPNGSSHARED_EXPORT StatisticsReads
{
public:

	///Read direction.
	enum ReadDirection
	{
		FORWARD, ///<Read 1
		REVERSE  ///<Read 2
	};

	///Constructor.
	StatisticsReads(bool long_read=false);
	///Updates the statistics based on the given read
	void update(const FastqEntry& entry, ReadDirection direction);
	///Updates the statistics based on the given alignment
	void update(const BamAlignment& al);

	///Returns the statistics result.
	QCCollection getResult();

private:
	long long c_forward_;
	long long c_reverse_;
	QMap<int,long long> read_lengths_;
    long long bases_sequenced_;
	long long c_read_q20_;
	long long c_base_q20_;
	long long c_base_q30_;
	QVector<Pileup> pileups_;
	QVector<double> qualities1_;
	QVector<double> qualities2_;
	Histogram qscore_dist_r1;
	Histogram qscore_dist_r2;
	bool long_read_;
	QVector<long long> base_qualities_;
	QVector<long long> read_qualities_;
};

#endif // STATISTICSREADS_H
