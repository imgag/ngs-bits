#include "FastqWriter.h"
#include <QThread>

FastqWriter::FastqWriter(const AnalysisJob& job, OutputStreams& streams, const TrimmingParameters& params, bool r1)
	: QObject()
	, QRunnable()
	, job_(job)
	, streams_(streams)
	, params_(params)
	, r1_(r1)
{
}

FastqWriter::~FastqWriter()
{
}

void FastqWriter::run()
{
	try
	{
		for (int r=0; r<job_.read_count; ++r)
		{
			if (job_.r1[r].bases.count()>=params_.min_len && job_.r2[r].bases.count()>=params_.min_len)
			{
				if (r1_) streams_.ostream1->write(job_.r1[r]);
				else streams_.ostream2->write(job_.r2[r]);
			}
		}
	}
	catch(Exception& e)
	{
		if (r1_) streams_.ostream1_error = e.message();
		else streams_.ostream2_error = e.message();
	}

	if (r1_) streams_.ostream1_done = true;
	else streams_.ostream2_done = true;
}
