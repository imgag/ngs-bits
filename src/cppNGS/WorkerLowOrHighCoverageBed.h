#ifndef WORKERLOWORHIGHCOVERAGEBED_H
#define WORKERLOWORHIGHCOVERAGEBED_H

#include <QRunnable>
#include "BedFile.h"
#include "BamReader.h"

class WorkerLowOrHighCoverageBed : public QRunnable
{
public:
	struct BedChunk
	{
		const BedFile& data;
		int start;
		int end;
		QString error; //In case of error
		BedFile output;

		void operator=(const BedChunk& bed_chunk)
		{
			if (&data != &bed_chunk.data)
			{
				THROW(NotImplementedException, "BedChunk data cannot be reassigned");
			}
			start = bed_chunk.start;
			end = bed_chunk.end;
			error = bed_chunk.error;
			output = bed_chunk.output;
		}
	};

	WorkerLowOrHighCoverageBed(BedChunk& bed_chunk, QString bam_file, int cutoff, int min_mapq, int min_baseq, QString ref_file, bool is_high);
	virtual void run() override;

private:
	BedChunk& bed_chunk_;
	QString bam_file_;
	int cutoff_;
	int min_mapq_;
	int min_baseq_;
	QString ref_file_;
	bool is_high_;
};

#endif // WORKERLOWORHIGHCOVERAGEBED_H