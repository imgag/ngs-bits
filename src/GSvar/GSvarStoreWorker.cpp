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
	variants_.store(filename_);
}
