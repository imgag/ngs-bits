#include "GSvarStoreWorker.h"
#include <QDebug>

GSvarStoreWorker::GSvarStoreWorker(const VariantList& variants, QString filename)
	: WorkerBase("Storing GSvar")
	, variants_(variants)
	, filename_(filename)
{
}

GSvarStoreWorker::~GSvarStoreWorker()
{
}

void GSvarStoreWorker::process()
{
	//make backup
	QString backup = filename_ + ".backup";
	QFile::copy(filename_, backup);

	//store
	variants_.store(filename_);

	//remove backup
	QFile::remove(backup);
}
