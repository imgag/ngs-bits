#ifndef WORKERLOWORHIGHCOVERAGECHR_H
#define WORKERLOWORHIGHCOVERAGECHR_H

#include <QRunnable>
#include "BedFile.h"
#include "BamReader.h"

class WorkerLowOrHighCoverageChr : public QRunnable
{
public:
	struct ChrChunk
	{
		Chromosome chr;
		int start;
		int end;
		QString error;
		BedFile output;

		void operator=(const ChrChunk& chr_chunk)
		{
			chr = chr_chunk.chr;
			start = chr_chunk.start;
			end = chr_chunk.end;
			error = chr_chunk.error;
			output = chr_chunk.output;
		}
	};

	WorkerLowOrHighCoverageChr(ChrChunk& chr_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high);
	virtual void run() override;

private:
	ChrChunk& chr_chunk_;
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;
	bool is_high_;
};


#endif // WORKERLOWORHIGHCOVERAGECHR_H
