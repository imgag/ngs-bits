#include "OutputWorker.h"

#include <Helper.h>

OutputWorker::OutputWorker(QList<AnalysisJob>& job_pool, QString output_filename)
	: QRunnable()
	, terminate_(false)
	, job_pool_(job_pool)
	, out_p_(Helper::openFileForWriting(output_filename, true))
	, write_chunk_(0)
{
}

void OutputWorker::run()
{
	while(!terminate_)
	{
		for (int j=0; j<job_pool_.count(); ++j)
		{
			AnalysisJob& job = job_pool_[j];
			if (job.status!=TO_BE_WRITTEN) continue;
			if (job.chunk_id!=write_chunk_) continue;

			try
			{
				foreach(const QByteArray& line, job.current_chunk_processed)
				{
					int bytes_written = out_p_->write(line);
					if (bytes_written==-1) THROW(FileAccessException, "Could not write output: " +  out_p_->errorString());
				}
				++write_chunk_;
				job.status = DONE;
			}
			catch(Exception& e)
			{
				job.error_message = e.message();
				job.status = ERROR;
				terminate();
			}
		}
	}

	//close output file
	out_p_->close();
}
