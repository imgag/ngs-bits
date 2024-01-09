#include "NGSDCacheInitializer.h"
#include "NGSD.h"

NGSDCacheInitializer::NGSDCacheInitializer()
	: BackgroundWorkerBase("NGSD cache initializer")
{
}

void NGSDCacheInitializer::process()
{
	NGSD db;
	db.transcripts();
}
