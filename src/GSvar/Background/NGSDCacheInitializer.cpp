#include "NGSDCacheInitializer.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include "NGSHelper.h"

NGSDCacheInitializer::NGSDCacheInitializer()
	: BackgroundWorkerBase("NGSD cache initializer")
{
}

void NGSDCacheInitializer::process()
{
	//load Ensembl-Refseq matches from resources
	NGSHelper::transcriptMatches();

	NGSD db;

	//init base gene infos
	db.approvedGeneNames();

	//init transcript infos
	db.transcripts();

	//init phenotype infos
	db.phenotypes(QStringList());

	//init relevant transcripts cache
	GSvarHelper::relevantTranscripts();
}
