#ifndef WORKERLOWCOVERAGECHR_H
#define WORKERLOWCOVERAGECHR_H

#include <QRunnable>
#include "BedFile.h"
#include "BamReader.h"

class WorkerLowCoverageChr : public QRunnable
{
public:
	struct ChrChunk
	{
		const Chromosome& chr;
		int start;
		int end;
		QString error;
		BedFile output;

		ChrChunk& operator=(const ChrChunk& /*item*/)
		{
			return *this;
		}
	};

	WorkerLowCoverageChr(ChrChunk& chr_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file);
	virtual void run() override;

private:
	ChrChunk& chr_chunk_;
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;

	void countCoverageWGSWithBaseQuality(int min_baseq, QVector<unsigned char>& cov, int start, int end, QBitArray& baseQualities, const BamAlignment& al);
	void countCoverageWGSWithoutBaseQuality(int start, int end, QVector<unsigned char>& cov);
};


#endif // WORKERLOWCOVERAGECHR_H
