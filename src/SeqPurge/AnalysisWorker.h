#ifndef ANALYSISWORKER_H
#define ANALYSISWORKER_H

#include <QRunnable>
#include <Auxilary.h>

///Fastq writer worker for threads
class AnalysisWorker
        : public QRunnable
{
public:
	AnalysisWorker(FastqEntry* e1_, FastqEntry* e2_, TrimmingParameters& params, TrimmingStatistics& stats, TrimmingData& data);
	~AnalysisWorker();
    void run();

	//initializes faktorial cache
	static void precalculateFactorials();

private:
	FastqEntry* e1_;
	FastqEntry* e2_;
	TrimmingParameters& params_;
	TrimmingStatistics& stats_;
	TrimmingData& data_;

	///Factorial calulation
	static double fak(int number);
	static QVector<double> fak_cache;

	///Match probability calulation
	static double matchProbability(int matches, int mismatches);

	///Helper function for writing output in threads
	void writeWithThread(FastqOutfileStream* stream, FastqEntry& entry);
};

#endif


