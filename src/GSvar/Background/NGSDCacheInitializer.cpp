#include "NGSDCacheInitializer.h"
#include "NGSD.h"

NGSDCacheInitializer::NGSDCacheInitializer()
	: BackgroundWorkerBase("NGSD cache initializer")
{
}

void NGSDCacheInitializer::process()
{
	NGSD db;

	//init base gene infos
	db.approvedGeneNames();

	//init transcript infos
	db.transcripts();

	//init phenotype infos
	db.phenotypes(QStringList());
}
