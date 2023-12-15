#include "BackgroundWorkerBase.h"
#include <QDebug>

BackgroundWorkerBase::BackgroundWorkerBase(QString name)
	: QObject()
	, QRunnable()
	, name_(name)
	, id_(-1)
{
}

QString BackgroundWorkerBase::name()
{
	return name_;
}

void BackgroundWorkerBase::setId(int id)
{
	id_= id;
}
