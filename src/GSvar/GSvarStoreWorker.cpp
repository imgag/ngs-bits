#include "GSvarStoreWorker.h"
#include <QDateTime>
#include <QFileInfo>

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
	//make sure temporary file does not exist (i.e. another GSvarStoreWorker is running)
	QString tmp = filename_ + ".tmp";
	while(QFile::exists(tmp))
	{
		//delete the tmp file if it was not modified for a minute (to avoid dead-locks in case a writer crashed)
		bool older_than_60s = QFileInfo(tmp).lastModified() < QDateTime::currentDateTime().addSecs(-60);
		if (older_than_60s)
		{
			QFile::remove(tmp);
		}

		//wait a bit
		QThread::sleep(5);
	}

	//store to temporary file
	variants_.store(tmp, VariantListFormat::TSV);

	//copy temp
	QFile::remove(filename_);
	QFile::rename(tmp, filename_);
}
