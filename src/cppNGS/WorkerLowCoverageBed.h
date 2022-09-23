#ifndef WORKERLOWCOVERAGEBED_H
#define WORKERLOWCOVERAGEBED_H

#include <QRunnable>
#include "BedFile.h"
#include "BamReader.h"
#include "StatHelper.h"

class WorkerLowCoverageBed : public QRunnable
{
public:
	struct BedChunk
	{
		BedFile& data;
		int start;
		int end;
		QString error; //In case of error
		BedFile output;

		void operator=(const BedChunk& bed_chunk)
		{
			data = bed_chunk.data;
			start = bed_chunk.start;
			end = bed_chunk.end;
			error = bed_chunk.error;
			output = bed_chunk.output;
		}
	};

	WorkerLowCoverageBed(BedChunk& bed_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file);
	virtual void run() override;

private:
	BedChunk& bed_chunk_;
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;
};

#endif // WORKERLOWCOVERAGEBED_H
