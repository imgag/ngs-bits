#include "WorkerBase.h"
#include "Exceptions.h"
#include "Log.h"

WorkerBase::WorkerBase(QString name)
	: QObject(0)
	, thread_(new QThread(0))
	, error_message_()
{
	setObjectName(name);
	moveToThread(thread_);
	connect(thread_, SIGNAL(started()), this, SLOT(processInternal()));
}

QString WorkerBase::errorMessage() const
{
	return error_message_;
}

void WorkerBase::start()
{
	thread_->start();
}

void WorkerBase::deleteLater()
{
	QObject::deleteLater();
	thread_->quit();
	thread_->wait();
}

void WorkerBase::processInternal()
{
	//start timer
	QTime timer;
	timer.start();

	//process
	try
	{
		process();
	}
	catch (Exception& e)
	{
		error_message_ = "Error: " + e.message();
	}
	catch (std::exception& e)
	{
		error_message_ = "Error: " + QString::fromLatin1(e.what());
	}
	catch (...)
	{
		error_message_ = "Unknown error";
	}

	//emit finished signal
	emit finished(error_message_=="");

	//log time
	Log::perf(objectName() + " took ", timer);
}
