#include "CacheInitWorker.h"
#include "NGSD.h"
#include "Settings.h"

CacheInitWorker::CacheInitWorker()
	: WorkerBase("Cache initialization")
{
}

void CacheInitWorker::process()
{
	QTime timer;
	timer.start();
	if (Settings::boolean("NGSD_enabled"))
	{
		NGSD().transcripts();
	}
	Log::perf("Initializing NGSD cache took ", timer);
}
