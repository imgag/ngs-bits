#include "CacheInitWorker.h"
#include "NGSD.h"
#include "Settings.h"
#include "GlobalServiceProvider.h"

CacheInitWorker::CacheInitWorker()
	: WorkerBase("Cache initialization")
{
}

void CacheInitWorker::process()
{
	if (GlobalServiceProvider::database().enabled())
	{
		NGSD().transcripts();
	}
}
