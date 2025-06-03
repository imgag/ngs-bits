#include "ThreadCoordinator.h"
#include "Exceptions.h"
#include "InputWorker.h"
#include "ChunkProcessor.h"
#include "OutputWorker.h"
#include "Helper.h"

ThreadCoordinator::ThreadCoordinator(QObject* parent, Parameters params, MetaData meta)
	: QObject(parent)
	, params_(params)
	, meta_(meta)
{
	//time duration of annotation
	timer_annotation_.start();

	//set number of threads
	thread_pool_read_.setMaxThreadCount(1);
	thread_pool_annotate_.setMaxThreadCount(params_.threads);
	thread_pool_write_.setMaxThreadCount(1);

	//create analysis job pool
	for (int i=0; i<params_.prefetch; ++i)
	{
		AnalysisJob job;
		job.index = i;
		job_pool_ << job;
	}

	//open output stream
	out_stream_ = Helper::openFileForWriting(params_.out, true);

	//open input steam
	FILE* instream = params_.in.isEmpty() ? stdin : fopen(params_.in.toLatin1().data(), "rb");
	if (instream==nullptr) THROW(FileAccessException, "Could not open file '" + params_.in + "' for reading!");
	in_stream_ = gzdopen(fileno(instream), "rb"); //always open in binary mode because windows and mac open in text mode
	if (in_stream_==nullptr) THROW(FileAccessException, "Could not open file '" + params_.in + "' for reading!");

	//initially fill thread pool with analysis jobs
	for (int i=0; i<params_.prefetch; ++i)
	{
		read(i);
	}
}

ThreadCoordinator::~ThreadCoordinator()
{
    if (params_.debug) QTextStream(stdout) << "Destroying ThreadCoordinator" << QT_ENDL;
}

void ThreadCoordinator::read(int i)
{
    if (params_.debug) QTextStream(stdout) << "ThreadCoordinator::read(" << i << ")" << QT_ENDL;

	InputWorker* worker = new InputWorker(job_pool_[i], in_stream_, params_);
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	connect(worker, SIGNAL(done(int)), this, SLOT(annotate(int)));
	connect(worker, SIGNAL(inputDone(int)), this, SLOT(inputDone(int)));
	thread_pool_read_.start(worker);
}

void ThreadCoordinator::annotate(int i)
{
    if (params_.debug) QTextStream(stdout) << "ThreadCoordinator::annotate(" << i << ")" << QT_ENDL;

	ChunkProcessor* worker = new ChunkProcessor(job_pool_[i], meta_, params_);

	connect(worker, SIGNAL(done(int)), this, SLOT(write(int)));
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	connect(worker, SIGNAL(log(int,int)), this, SLOT(update_stats(int,int)));
	thread_pool_annotate_.start(worker);
}

void ThreadCoordinator::write(int i)
{
    if (params_.debug) QTextStream(stdout) << "ThreadCoordinator::write(" << i << ")" << QT_ENDL;

	OutputWorker* worker = new OutputWorker(job_pool_[i], out_stream_, params_);
	connect(worker, SIGNAL(error(int,QString)), this, SLOT(error(int,QString)));
	connect(worker, SIGNAL(retry(int)), this, SLOT(write(int)));
	connect(worker, SIGNAL(done(int)), this, SLOT(read(int)));
	thread_pool_write_.start(worker);
}

void ThreadCoordinator::error(int /*i*/, QString message)
{
	THROW(Exception, message);
}

void ThreadCoordinator::update_stats(int annotated, int skipped)
{
	c_mutex.lock();
	c_annotated_ += annotated;
	c_skipped_ += skipped;
	c_mutex.unlock();
}

void ThreadCoordinator::inputDone(int /*i*/)
{
	//nothing to do
	if (input_done_) return;

	//start timer to regularly check if all jobs are processed
	input_done_ = true;
	connect(&timer_done_, SIGNAL(timeout()), this, SLOT(checkDone()));
	timer_done_.start(100);

    QTextStream(stdout) << "Reading input done" << QT_ENDL;
}

void ThreadCoordinator::checkDone()
{
    if (params_.debug) QTextStream(stdout) << "ThreadCoordinator::checkDone()" << QT_ENDL;

	//check if all jobs are done
	for (int i=0; i<job_pool_.count(); ++i)
	{
		if (job_pool_[i].chunk_nr!=-1) return;
	}

	//done > stop timer to prevent it from fireing again
	timer_done_.stop();

	QTextStream stream(stdout);
    stream << "Annotation done" << QT_ENDL;
    stream << "Annotated " << QString::number(c_annotated_) << " variants." << QT_ENDL;
    stream << "Skipped " << QString::number(c_skipped_) << " invalid variants." << QT_ENDL;
    stream << "Annotation took: " << Helper::elapsedTime(timer_annotation_) << QT_ENDL;

	emit finished();
}
