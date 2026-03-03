#include "OutputHandler.h"
#include "Helper.h"

OutputHandler::OutputHandler(QTextStream& out_stream, QTextStream& debug_stream)
	:out_stream_(out_stream), debug_stream_(debug_stream), bams_done_(0)
{
	timer_.start();
}

void OutputHandler::debugMessage(QString msg)
{
	QMutexLocker lock(&debug_stream_mtx_);
	debug_stream_ << msg;
}

void OutputHandler::outputMessage(QString msg)
{
	QMutexLocker lock(&out_stream_mtx_);
	out_stream_ << msg;
}

void OutputHandler::bamDone()
{
	QMutexLocker lock(&bams_done_mtx_);
	++bams_done_;
	if (bams_done_ % 100 == 0) debugMessage((QTextStream() << "##Determining SNPs for 100 BAM/CRAM files took " << Helper::elapsedTime(timer_.restart()) << Qt::endl).readAll());
}
