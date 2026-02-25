#ifndef BAMWORKER_H
#define BAMWORKER_H

#include <QFile>
#include <QRunnable>
#include <QMutex>
#include <Exceptions.h>
#include "VcfFile.h"

typedef QList<signed char> AfData;

struct BamWorkerArgs{
	QString& bam;
	AfData& output_af;
	bool& is_valid;
	const VcfFile& snps;
	const QString& infile;


	QTextStream& out_stream;
	QMutex& out_stream_mtx;
	QTextStream& debug_stream;
	QMutex& debug_stream_mtx;

	QElapsedTimer& timer;
	int& bams_done;
	QMutex& bams_done_mtx;

	int min_depth;
	bool debug;
	bool time;
};

class BamWorker
	: public QRunnable
{
public:
	BamWorker(const BamWorkerArgs&);

	void run();

private:
	QString& bam_;
	AfData& output_af_;
	bool& is_valid_;
	const VcfFile& snps_;
	const QString& infile_;

	QTextStream& out_stream_;
	QMutex& out_stream_mtx_;
	QTextStream& debug_stream_;
	QMutex& debug_stream_mtx_;

	QElapsedTimer& timer_;
	int& bams_done_;
	QMutex& bams_done_mtx_;

	int min_depth_;

	//flags
	bool debug_;
	bool time_;
};


#endif // BAMWORKER_H
