#include "ThreadCoordinator.h"
#include "OutputWorker.h"
#include "AnalysisWorker.h"
#include "Helper.h"

ThreadCoordinator::ThreadCoordinator(QObject* parent, QStringList in1_files, QStringList in2_files, OutputStreams streams, TrimmingParameters params)
	: QObject(parent)
	, in1_files_(in1_files)
	, in2_files_(in2_files)
	, streams_(streams)
	, job_pool_()
	, params_(params)
	, stats_()
{
	//set number of threads
	QThreadPool::globalInstance()->setMaxThreadCount(params_.threads);

	//create analysis job pool
	for (int i=0; i<params_.prefetch; ++i)
	{
		AnalysisJob job;
		job.index = i;
		job_pool_ << job;
	}

	//initially fill thread pool with analysis jobs
	for (int i=0; i<params_.prefetch; ++i)
	{
		load(i);
	}

	//show progress via timer
	if (params_.progress>0)
	{
		printStatus();
		connect(&timer_progress_, SIGNAL(timeout()), this, SLOT(printStatus()));
		timer_progress_.start(params_.progress);
	}
}

ThreadCoordinator::~ThreadCoordinator()
{
	//QTextStream(stdout) << "~ThreadCoordinator" << endl;
}

void ThreadCoordinator::printStatus()
{
	int to_be_analyzed = 0;
	int to_be_written = 0;
	int done = 0;
	for (int i=0; i<job_pool_.count(); ++i)
	{
		if (job_pool_[i].status==DONE) ++done;
		if (job_pool_[i].status==TO_BE_ANALYZED) ++to_be_analyzed;
		if (job_pool_[i].status==TO_BE_WRITTEN) ++to_be_written;
	}
	(*streams_.summary_stream)<< Helper::dateTime() << " progress - to_be_analyzed: " << to_be_analyzed << " to_be_written: " << to_be_written << " done: " << done << endl;
}

void ThreadCoordinator::write(int i)
{
	//QTextStream(stdout) << "ThreadCoordinator::write " << i << endl;
	OutputWorker* worker = new OutputWorker(job_pool_[i], streams_, params_, stats_);
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	connect(worker, SIGNAL(done(int)), this, SLOT(load(int)));
	QThreadPool::globalInstance()->start(worker, 3);
}

void ThreadCoordinator::load(int i)
{
	//QTextStream(stdout) << "ThreadCoordinator::load " << i << endl;

	//nothing to read anymore
	if (timer_done_.isActive()) return;

	AnalysisJob& job = job_pool_[i];

	job.clear();
	if (!readPair(job))
	{
		//QTextStream(stdout) << "########################### READING DONE ###########################" << i << endl;
		connect(&timer_done_, SIGNAL(timeout()), this, SLOT(checkDone()));
		timer_done_.start(100);
		return;
	}
	job.status = TO_BE_ANALYZED;

	AnalysisWorker* worker = new AnalysisWorker(job, params_, stats_, ec_stats_);
	connect(worker, SIGNAL(done(int)), this, SLOT(write(int)));
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	QThreadPool::globalInstance()->start(worker, 1);
}

void ThreadCoordinator::error(int /*i*/, QString message)
{
	//QTextStream(stdout) << "ThreadCoordinator::error " << i << " " << message << endl;
	THROW(Exception, message);
}

void ThreadCoordinator::checkDone()
{
	//QTextStream(stdout) << Helper::dateTime() << " ThreadCoordinator::checkDone" << endl;

	//check if all jobs are done
	for (int i=0; i<job_pool_.count(); ++i)
	{
		if (job_pool_[i].status!=DONE) return;
	}

	//done > stop timer to prevent it from fireing again
	timer_done_.stop();

	//print trimming statistics
	(*streams_.summary_stream) << Helper::dateTime() << " writing statistics summary" << endl;
	stats_.writeStatistics((*streams_.summary_stream), params_);

	//write qc output file
	if (!params_.qc.isEmpty())
	{
		stats_.qc.getResult().storeToQCML(params_.qc, QStringList() << in1_files_ << in1_files_, "");
	}

	//print error correction statistics
	if (params_.ec)
	{
		if (params_.progress>0) (*streams_.summary_stream) << Helper::dateTime() << " writing error corrections summary" << endl;
		ec_stats_.writeStatistics((*streams_.summary_stream));
	}

	emit finished();
}

bool ThreadCoordinator::readPair(AnalysisJob& job)
{
	static int index = -1;
	static QSharedPointer<FastqFileStream> in1;
	static QSharedPointer<FastqFileStream> in2;

	if (index==-1 || (in1->atEnd() && in2->atEnd())) //init or both at end > open next input file pair
	{
		++index;
		if (index==in1_files_.count())
		{
			return false;
		}
		else
		{
			//QTextStream(stdout) << "ThreadCoordinator::readPair " << in1_files_.front() << " " << in2_files_.front() << endl;
			in1.reset(new FastqFileStream(in1_files_[index], false));
			in2.reset(new FastqFileStream(in2_files_[index], false));
		}
	}
	else if (in1->atEnd()) //read number different > error
	{
		THROW(FileParseException, "File " + in2->filename() + " has more entries than " + in1->filename() + "!");
	}
	else if (in2->atEnd()) //read number different > error
	{
		THROW(FileParseException, "File " + in1->filename() + " has more entries than " + in2->filename() + "!");
	}

	//read data
	in1->readEntry(job.e1);
	in2->readEntry(job.e2);

	return true;
}
