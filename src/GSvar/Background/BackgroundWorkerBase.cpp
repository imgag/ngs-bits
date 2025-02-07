#include "BackgroundWorkerBase.h"
#include "Exceptions.h"
#include <QElapsedTimer>

BackgroundWorkerBase::BackgroundWorkerBase(QString name)
	: QObject()
	, QRunnable()
	, name_(name)
	, id_(-1)
{
	setAutoDelete(false);
}

QString BackgroundWorkerBase::name()
{
	return name_;
}

void BackgroundWorkerBase::setId(int id)
{
	id_= id;
}

int BackgroundWorkerBase::id() const
{
	return id_;
}

int BackgroundWorkerBase::elapsed() const
{
	return elapsed_ms_;
}

const QString& BackgroundWorkerBase::error() const
{
	return error_;
}

void BackgroundWorkerBase::run()
{
    QElapsedTimer timer;
	timer.start();

	try
	{
		emit started();

		process();

		elapsed_ms_ = timer.elapsed();
		emit finished();
	}
	catch (Exception& e)
	{
		elapsed_ms_ = timer.elapsed();
		error_ = e.message();
		emit failed();
	}

}

void BackgroundWorkerBase::userInteration()
{
}
