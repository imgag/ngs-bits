#include "InputWorker.h"

InputWorker::InputWorker(AnalysisJob& job, InputStreams& streams, const TrimmingParameters& params)
	: QObject()
	, QRunnable()
	, job_(job)
	, streams_(streams)
	, params_(params)
{
}

InputWorker::~InputWorker()
{
}

void InputWorker::run()
{
	try
	{
		job_.clear();

		bool end_of_data_reached = false;
		int pairs_read = 0;
		while(pairs_read<params_.block_size && !end_of_data_reached)
		{
			if (streams_.istream1->atEnd() && streams_.istream2->atEnd()) //both at end > open next input file pair
			{
				++streams_.current_index;
				if (streams_.current_index>=params_.files_in1.count())
				{
					end_of_data_reached = true;
				}
				else
				{					
					streams_.istream1.reset(new FastqFileStream(params_.files_in1[streams_.current_index], false));
					streams_.istream2.reset(new FastqFileStream(params_.files_in2[streams_.current_index], false));
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
			if (!end_of_data_reached)
			{
				streams_.istream1->readEntry(job_.r1[pairs_read]);
				streams_.istream2->readEntry(job_.r2[pairs_read]);
				++pairs_read;
			}
		}

		if (pairs_read>0)
		{
			job_.status = TO_BE_ANALYZED;
			job_.read_count = pairs_read;
			emit done(job_.index);
		}
		else
		{
			job_.status = DONE;
		}

		if (end_of_data_reached)
		{
			emit inputDone(job_.index);
		}
	}
	catch(Exception& e)
	{		
		emit error(job_.index, e.message());
	}
}
