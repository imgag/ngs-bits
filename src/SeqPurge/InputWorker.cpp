#include <QThread>
#include "InputWorker.h"

InputWorker::InputWorker(AnalysisJob& job, InputStreams& streams)
	: QObject()
	, QRunnable()
	, job_(job)
	, streams_(streams)
{
	//QTextStream(stdout) << "InputWorker" << endl;
}

InputWorker::~InputWorker()
{
	//QTextStream(stdout) << "~InputWorker" << endl;
}

void InputWorker::run()
{
	//QTextStream(stdout) << "InputWorker:run " << job_.index << " thread: " << QThread::currentThreadId() << endl;

	try
	{
		job_.clear();

		if (streams_.istream1->atEnd() && streams_.istream2->atEnd()) //init or both at end > open next input file pair
		{
			++streams_.current_index;
			if (streams_.current_index>=streams_.files_in1.count())
			{
				emit inputDone(job_.index);
				return;
			}
			else
			{
				//QTextStream(stdout) << "InputWorker::run new file pair with index " << streams_.current_index << endl;
				streams_.istream1.reset(new FastqFileStream(streams_.files_in1[streams_.current_index], false));
				streams_.istream2.reset(new FastqFileStream(streams_.files_in2[streams_.current_index], false));
			}
		}
		else if (streams_.istream1->atEnd()) //read number different > error
		{
			THROW(FileParseException, "File " + streams_.istream2->filename() + " has more entries than " + streams_.istream1->filename() + "!");
		}
		else if (streams_.istream2->atEnd()) //read number different > error
		{
			THROW(FileParseException, "File " + streams_.istream1->filename() + " has more entries than " + streams_.istream2->filename() + "!");
		}

		//read data
		streams_.istream1->readEntry(job_.e1);
		streams_.istream2->readEntry(job_.e2);
		job_.status = TO_BE_ANALYZED;

		emit done(job_.index);
	}
	catch(Exception& e)
	{
		//QTextStream(stdout) << "InputWorker:error " << job_.index << " thread:" << QThread::currentThreadId() << " message:" << e.message() << endl;
		emit error(job_.index, e.message());
	}
}
