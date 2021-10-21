#include "CacheInitWorker.h"
#include "NGSD.h"
#include "Settings.h"

CacheInitWorker::CacheInitWorker()
	: WorkerBase("Cache initialization")
{
}

void CacheInitWorker::process()
{
	if (Settings::boolean("NGSD_enabled"))
	{
		NGSD().transcripts();
	}
}
