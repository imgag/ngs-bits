#include "OutputWorker.h"
#include "Helper.h"
#include <QThread>

OutputWorker::OutputWorker(AnalysisJob& job, QSharedPointer<QFile> out_stream)
	: QRunnable()
	, job_(job)
	, out_stream_(out_stream)
{
}

void OutputWorker::run()
{
	//static variables shared between output workers
	static int write_chunk = 0;

	try
	{
		if (job_.chunk_nr!=write_chunk)
		{
			emit retry(job_.index);
			return;
		}

		foreach(const QByteArray& line, job_.lines)
		{
			int bytes_written = out_stream_->write(line);
			if (bytes_written==-1) THROW(FileAccessException, "Could not write output: " +  out_stream_->errorString());
		}
		++write_chunk;


		emit done(job_.index);
	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message());
	}
}
