#include "OutputWorker.h"
#include <QThread>

OutputWorker::OutputWorker(AnalysisJob& job, OutputStreams& streams, const TrimmingParameters& params, TrimmingStatistics& stats)
	: QObject()
	, QRunnable()
	, job_(job)
	, streams_(streams)
	, params_(params)
	, stats_(stats)
{
	//QTextStream(stdout) << "OutputWorker" << endl;
}

OutputWorker::~OutputWorker()
{
	//QTextStream(stdout) << "~OutputWorker" << endl;
}

void OutputWorker::run()
{
	//QTextStream(stdout) << "OutputWorker:run " << job_.index << " thread: " << QThread::currentThreadId() << endl;
	try
	{
		//write output
		int reads_removed = 0;
		for (int r=0; r<job_.read_count; ++r)
		{
			if (job_.r1[r].bases.count()>=params_.min_len && job_.r2[r].bases.count()>=params_.min_len)
			{
				streams_.ostream1->write(job_.r1[r]);
				streams_.ostream2->write(job_.r2[r]);
			}
			else if (!streams_.ostream3.isNull() && job_.r1[r].bases.count()>=params_.min_len)
			{
				reads_removed += 1;
				streams_.ostream3->write(job_.r1[r]);
			}
			else if (!streams_.ostream4.isNull() && job_.r2[r].bases.count()>=params_.min_len)
			{
				reads_removed += 1;
				streams_.ostream4->write(job_.r2[r]);
			}
			else
			{
				reads_removed += 2;
			}
		}

		//update statistics
		stats_.read_num += 2*job_.read_count;
		stats_.reads_trimmed_insert += job_.reads_trimmed_insert;
		stats_.reads_trimmed_adapter += job_.reads_trimmed_adapter;
		stats_.reads_trimmed_n += job_.reads_trimmed_n;
		stats_.reads_trimmed_q += job_.reads_trimmed_q;
		stats_.reads_removed += reads_removed;
		for (int r=0; r<job_.read_count; ++r)
		{
			stats_.bases_remaining[job_.r1[r].bases.length()] += 1;
			stats_.bases_remaining[job_.r2[r].bases.length()] += 1;
			if (job_.length_r1_orig[r]>0)
			{
				stats_.bases_perc_trim_sum += (double)(job_.length_r1_orig[r] - job_.r1[r].bases.count()) / job_.length_r1_orig[r];
			}
			if (job_.length_r2_orig[r]>0)
			{
				stats_.bases_perc_trim_sum += (double)(job_.length_r2_orig[r] - job_.r2[r].bases.count()) / job_.length_r2_orig[r];
			}
		}

		//mark job as done
		emit done(job_.index);
		job_.status = DONE;
	}
	catch(Exception& e)
	{
		//QTextStream(stdout) << "OutputWorker:error " << job_.index << " thread:" << QThread::currentThreadId() << " message:" << e.message() << endl;
		emit error(job_.index, e.message());
	}
}
