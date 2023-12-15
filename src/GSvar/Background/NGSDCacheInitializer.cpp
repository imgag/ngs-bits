#include "NGSDCacheInitializer.h"
#include "NGSD.h"

NGSDCacheInitializer::NGSDCacheInitializer()
	: BackgroundWorkerBase("NGSD cache initializer")
{
}

void NGSDCacheInitializer::run()
{
	QTime timer;
	timer.start();

	try
	{
		emit started(id_);

		NGSD db;
		db.transcripts();

		emit finished(id_, timer.elapsed());
	}
	catch (Exception& e)
	{
		emit failed(id_, timer.elapsed(), e.message());
	}
}
