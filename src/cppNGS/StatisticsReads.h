#ifndef STATISTICSREADS_H
#define STATISTICSREADS_H

#include "cppNGS_global.h"
#include <QSet>
#include "FastqFileStream.h"
#include "QCCollection.h"
#include "Pileup.h"
#include <QMutex>

/*
	@brief Read statistics for quality control.

	@note This class is thread-safe, i.e. several threads can update statistics without corrupting the result.
*/
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
	StatisticsReads();
	///Updates the statistics based on the given read.
	void update(const FastqEntry& entry, ReadDirection direction);
	///Returns the statistics result.
	QCCollection getResult();

private:
	long long c_forward_;
	long long c_reverse_;
	QSet<int> read_lengths_;
    long long bases_sequenced_;
	long long c_read_q20_;
	long long c_base_q30_;
	QVector<Pileup> pileups_;
	QVector<double> qualities1_;
	QVector<double> qualities2_;
};

#endif // STATISTICSREADS_H
