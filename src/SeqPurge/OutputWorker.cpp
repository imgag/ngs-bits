#include "OutputWorker.h"
#include "FastqWriter.h"
#include <QThread>
#include <QTime>

OutputWorker::OutputWorker(AnalysisJob& job, OutputStreams& streams, const TrimmingParameters& params, TrimmingStatistics& stats)
	: QObject()
	, QRunnable()
	, job_(job)
	, streams_(streams)
	, params_(params)
	, stats_(stats)
{
}

OutputWorker::~OutputWorker()
{
}

void OutputWorker::run()
{
	try
	{
		//write paired reads in separate threads
		streams_.ostream1_done = false;
		streams_.ostream1_error.clear();
		FastqWriter* worker = new FastqWriter(job_, streams_, params_, true);
		streams_.ostream1_thread.start(worker);

		streams_.ostream2_done = false;
		streams_.ostream2_error.clear();
		worker = new FastqWriter(job_, streams_, params_, false);
		streams_.ostream2_thread.start(worker);

		//write unpaired reads
		int reads_removed = 0;
		for (int r=0; r<job_.read_count; ++r)
		{
			if (job_.r1[r].bases.count()>=params_.min_len && job_.r2[r].bases.count()>=params_.min_len)
			{
				//nothing to do here as they are written in parallel by separate threads (see above)
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

		//wait until writing is done
		while(!streams_.ostream1_done || !streams_.ostream2_done)
		{
			QThread::usleep(1);
		}

		//handle errors
		if (!streams_.ostream1_error.isEmpty())
		{
			THROW(Exception, streams_.ostream1_error);
		}
		if (!streams_.ostream2_error.isEmpty())
		{
			THROW(Exception, streams_.ostream2_error);
		}

		//mark job as done
		job_.status = DONE;
		emit done(job_.index);
	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message());
	}
}
