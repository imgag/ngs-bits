#include "OutputWorker.h"

OutputWorker::OutputWorker(AnalysisJob& job, QSharedPointer<QFile> out_stream, Parameters& params)
	: QRunnable()
	, job_(job)
	, out_stream_(out_stream)
	, params_(params)
{
    if (params_.debug) QTextStream(stdout) << "OutputWorker(): " << job_.index << QT_ENDL;
}

OutputWorker::~OutputWorker()
{
    if (params_.debug) QTextStream(stdout) << "~OutputWorker(): " << job_.index << QT_ENDL;
}


void OutputWorker::run()
{
	//static variables shared between output workers
	static int write_chunk = 0;

	try
	{
		if (job_.chunk_nr!=write_chunk)
		{
            if (params_.debug) QTextStream(stdout) << "OutputWorker::run() job: " << job_.index << " - Cannot write now wrong order. Job has chunk: " << job_.chunk_nr << " next chunk is " << write_chunk << QT_ENDL;
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
