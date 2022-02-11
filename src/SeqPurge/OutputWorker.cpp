#include "OutputWorker.h"

OutputWorker::OutputWorker(QString out1, QString out2, QString out3_base, const TrimmingParameters& params, TrimmingStatistics& stats)
	: QObject()
	, out_(stdout)
	, ostream1(new FastqOutfileStream(out1, params.compression_level))
	, ostream2(new FastqOutfileStream(out2, params.compression_level))
	, ostream3()
	, ostream4()
	, params_(params)
	, stats_(stats)
{
	if (out3_base.trimmed()!="")
	{
		ostream3.reset(new FastqOutfileStream(out3_base + "_R1.fastq.gz", params.compression_level));
		ostream4.reset(new FastqOutfileStream(out3_base + "_R2.fastq.gz", params.compression_level));
	}
}

void OutputWorker::write(AnalysisJob* job)
{
	try
	{
		out_ << "OutputWorker: writing...\n";

		//write output
		int reads_removed = 0;
		if (job->e1.bases.count()>=params_.min_len && job->e2.bases.count()>=params_.min_len)
		{
			ostream1->write(job->e1);
			ostream2->write(job->e2);
		}
		else if (!ostream3.isNull() && job->e1.bases.count()>=params_.min_len)
		{
			reads_removed += 1;
			ostream3->write(job->e1);
		}
		else if (!ostream4.isNull() && job->e2.bases.count()>=params_.min_len)
		{
			reads_removed += 1;
			ostream4->write(job->e2);
		}
		else
		{
			reads_removed += 2;
		}

		//update statistics
		stats_.read_num += 2;
		stats_.reads_trimmed_insert += job->reads_trimmed_insert;
		stats_.reads_trimmed_adapter += job->reads_trimmed_adapter;
		stats_.reads_trimmed_n += job->reads_trimmed_n;
		stats_.reads_trimmed_q += job->reads_trimmed_q;
		stats_.reads_removed += reads_removed;
		stats_.bases_remaining[job->e1.bases.length()] += 1;
		stats_.bases_remaining[job->e2.bases.length()] += 1;
		if (job->length_s1_orig>0)
		{
			stats_.bases_perc_trim_sum += (double)(job->length_s1_orig - job->e1.bases.count()) / job->length_s1_orig;
		}
		if (job->length_s2_orig>0)
		{
			stats_.bases_perc_trim_sum += (double)(job->length_s2_orig - job->e2.bases.count()) / job->length_s2_orig;
		}

		//mark job as done
		job->status = DONE;

		out_ << "OutputWorker: write done\n";
	}
	catch(Exception& e)
	{
		out_ << "OutputWorker: write error\n";
		job->status = ERROR;
		job->error_message = e.message();
	}
}

void OutputWorker::threadStarted()
{
	out_ << "OutputWorker: thread started\n";
}

void OutputWorker::threadFinished()
{
	out_ << "OutputWorker: thread finished\n";
}





