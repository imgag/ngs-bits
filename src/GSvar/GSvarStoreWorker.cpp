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
	//store to temporary file
	QString tmp = filename_ + ".tmp";
	variants_.store(tmp, VariantListFormat::TSV);

	//copy temp
	QFile::remove(filename_);
	QFile::rename(tmp, filename_);
}
