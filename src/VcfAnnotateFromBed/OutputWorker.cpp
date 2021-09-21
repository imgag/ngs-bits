#include "OutputWorker.h"

#include <Helper.h>
#include <QThread>

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

			try
			{
				if (job.chunk_id == write_chunk_)
				{
					foreach(const QByteArray& line, job.current_chunk_processed)
					{
						out_p_->write(line);
					}

					++write_chunk_;
					out_p_->flush();
					job.clear();
					job.status = DONE;
				}
				else
				{
					//sleep
					QThread::msleep(200);
				}
			}
			catch(Exception& e)
			{
				job.status = ERROR;
				job.error_message = e.message();
				terminate();
			}
		}
	}

	//close output file
	out_p_->close();
}
