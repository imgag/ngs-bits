#include "ThreadCoordinator.h"
#include "InputWorker.h"
#include "OutputWorker.h"
#include "AnalysisWorker.h"
#include "Helper.h"

ThreadCoordinator::ThreadCoordinator(QObject* parent, InputStreams streams_in, OutputStreams streams_out, TrimmingParameters params)
	: QObject(parent)
	, streams_in_(streams_in)
	, streams_out_(streams_out)
	, job_pool_()
	, thread_pool_read_()
	, thread_pool_analyze_()
	, thread_pool_write_()
	, params_(params)
	, stats_()
{
	timer_overall_.start();

	//set number of threads
	thread_pool_read_.setMaxThreadCount(1);
	thread_pool_analyze_.setMaxThreadCount(params_.threads);
	thread_pool_write_.setMaxThreadCount(1);

	//create analysis job pool
	for (int i=0; i<params_.prefetch; ++i)
	{
		job_pool_ << AnalysisJob(i, params_.block_size);
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
	(*streams_out_.summary_stream)<< Helper::dateTime() << " progress - to_be_analyzed: " << to_be_analyzed << " to_be_written: " << to_be_written << " done: " << done << endl;
}

void ThreadCoordinator::load(int i)
{
	//QTextStream(stdout) << "ThreadCoordinator::load " << i << endl;
	InputWorker* worker = new InputWorker(job_pool_[i], streams_in_, params_);
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	connect(worker, SIGNAL(done(int)), this, SLOT(analyze(int)));
	connect(worker, SIGNAL(inputDone(int)), this, SLOT(inputDone(int)));
	thread_pool_read_.start(worker);
}

void ThreadCoordinator::analyze(int i)
{
	AnalysisWorker* worker = new AnalysisWorker(job_pool_[i], params_, stats_, ec_stats_);
	connect(worker, SIGNAL(done(int)), this, SLOT(write(int)));
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	thread_pool_analyze_.start(worker);
}

void ThreadCoordinator::write(int i)
{
	//QTextStream(stdout) << "ThreadCoordinator::write " << i << endl;
	OutputWorker* worker = new OutputWorker(job_pool_[i], streams_out_, params_, stats_);
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	connect(worker, SIGNAL(done(int)), this, SLOT(load(int)));
	thread_pool_write_.start(worker);
}

void ThreadCoordinator::error(int /*i*/, QString message)
{
	//QTextStream(stdout) << "ThreadCoordinator::error " << i << " " << message << endl;
	THROW(Exception, message);
}

void ThreadCoordinator::inputDone(int /*i*/)
{
	//QTextStream(stdout) << "ThreadCoordinator::inputDone" << endl;
	//timer already running > nothing to do
	if (timer_done_.isActive()) return;

	connect(&timer_done_, SIGNAL(timeout()), this, SLOT(checkDone()));
	timer_done_.start(100);
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
	(*streams_out_.summary_stream) << Helper::dateTime() << " writing statistics summary" << endl;
	stats_.writeStatistics((*streams_out_.summary_stream), params_);

	//write qc output file
	if (!params_.qc.isEmpty())
	{
		stats_.qc.getResult().storeToQCML(params_.qc, QStringList() << in1_files_ << in1_files_, "");
	}

	//print error correction statistics
	if (params_.ec)
	{
		if (params_.progress>0) (*streams_out_.summary_stream) << Helper::dateTime() << " writing error corrections summary" << endl;
		ec_stats_.writeStatistics((*streams_out_.summary_stream));
	}

	(*streams_out_.summary_stream) << Helper::dateTime() << " overall runtime: " << Helper::elapsedTime(timer_overall_) << endl;

	emit finished();
}

