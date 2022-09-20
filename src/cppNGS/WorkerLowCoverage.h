#ifndef WORKERLOWCOVERAGE_H
#define WORKERLOWCOVERAGE_H

#include <QRunnable>
#include "BedFile.h"
#include "BamReader.h"

class WorkerLowCoverage : public QRunnable
{
public:
	struct Chunk
	{
		const BedFile& data;
		int start;
		int end;
		QString error; //In case of error
		BedFile output;

		Chunk& operator=(const Chunk& /*item*/)
		{			
			return *this;
		}
	};

	WorkerLowCoverage(Chunk& chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file);
	virtual void run() override;

private:
	Chunk& chunk_;	
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;

	void countCoverageWithBaseQuality(
			int min_baseq,
			QVector<int>& roi_cov,
			int start,
			int ol_start,
			int ol_end,
			QBitArray& baseQualities,
			const BamAlignment& al);


	void countCoverageWithoutBaseQuality(
			QVector<int>& roi_cov,
			int ol_start,
			int ol_end);
};

#endif // WORKERLOWCOVERAGE_H
