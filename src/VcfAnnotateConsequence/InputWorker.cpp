#include "InputWorker.h"
#include "Exceptions.h"

InputWorker::InputWorker(AnalysisJob& job, QSharedPointer<VersatileFile> in_stream, Parameters& params)
	: QObject()
	, QRunnable()
	, job_(job)
	, in_stream_(in_stream)
	, params_(params)
{
}

InputWorker::~InputWorker()
{
}

void InputWorker::run()
{
	//static data shared between input workers
	static int current_chunk = 0;
	static bool reading_done = false;

	try
	{
		job_.clear();

		//reading input is done
		if (reading_done) return;

		//check if reading input is done
		if (in_stream_->atEnd())
		{
			reading_done = true;
			emit inputDone(job_.index);
			return;
		}

		//init job
		job_.chunk_nr = current_chunk++;

		//read lines
		while(!in_stream_->atEnd() && job_.lines.count() < params_.block_size)
		{
			job_.lines.append(in_stream_->readLine());
		}
        if (params_.debug) QTextStream(stdout) << "InputWorker(): " << job_.index << " job done!" << Qt::endl;
		emit done(job_.index);
	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message());
	}
}
