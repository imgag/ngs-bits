#include <QThread>
#include "InputWorker.h"
#include "Exceptions.h"

InputWorker::InputWorker(AnalysisJob& job, gzFile& in_stream, Parameters& params)
	: QObject()
	, QRunnable()
	, job_(job)
	, in_stream_(in_stream)
	, params_(params)
{
    if (params_.debug) QTextStream(stdout) << "InputWorker(): " << job_.index << QT_ENDL;
}

InputWorker::~InputWorker()
{
    if (params_.debug) QTextStream(stdout) << "~InputWorker(): " << job_.index << QT_ENDL;
}

void InputWorker::run()
{
	//static data shared between input workers
	static int current_chunk = 0;
	static const int buffer_size = 1048576; //1MB buffer
	static char* buffer = new char[buffer_size];
	static bool reading_done = false;

	try
	{
		job_.clear();

		//reading input is done
		if (reading_done) return;

		//check if reading input is done
		if (gzeof(in_stream_))
		{
			reading_done = true;
			emit inputDone(job_.index);
			return;
		}

		//init job
		job_.chunk_nr = current_chunk++;

		//read lines
		while(!gzeof(in_stream_) && job_.lines.count() < params_.block_size)
		{
			char* char_array = gzgets(in_stream_, buffer, buffer_size);

			//handle errors like truncated GZ file
			if (char_array==nullptr)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(in_stream_, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					THROW(FileParseException, "Error while reading input: " + error_message);
				}
			}

			job_.lines.append(QByteArray(char_array));
		}

		emit done(job_.index);
	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message());
	}
}
