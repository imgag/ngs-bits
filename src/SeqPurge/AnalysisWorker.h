#ifndef ANALYSISWORKER_H
#define ANALYSISWORKER_H

#include <QRunnable>
#include <QSharedPointer>
#include <Auxilary.h>

///Fastq writer worker for threads
class AnalysisWorker
        : public QRunnable
{
public:
	AnalysisWorker(QSharedPointer<FastqEntry> e1, QSharedPointer<FastqEntry> e2, TrimmingParameters& params, TrimmingStatistics& stats, ErrorCorrectionStatistics& ecstats, TrimmingData& data);
	~AnalysisWorker();
	void run();

	//initializes faktorial cache
	static void precalculateFactorials();

private:
	QSharedPointer<FastqEntry> e1_;
	QSharedPointer<FastqEntry> e2_;
	TrimmingParameters& params_;
	TrimmingStatistics& stats_;
	ErrorCorrectionStatistics& ecstats_;
	TrimmingData& data_;

	///Factorial calulation
	static double fak(int number);
	static QVector<double> fak_cache;

	///Match probability calulation
	static double matchProbability(int matches, int mismatches);

	///Check read headers match (input and output)
	static void checkHeaders(const QByteArray& h1, const QByteArray& h2);

	///Error correction
	void correctErrors(QTextStream& debug_out);
};

#endif


