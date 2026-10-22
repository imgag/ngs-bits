#include "NGSDCacheInitializer.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include "NGSHelper.h"
#include "Log.h"

NGSDCacheInitializer::NGSDCacheInitializer()
	: BackgroundWorkerBase("NGSD cache initializer")
{
}

void NGSDCacheInitializer::process()
{
	QElapsedTimer timer;
	timer.start();

	//load Ensembl-Refseq matches from resources
	NGSHelper::transcriptMatches();
	Log::perf("Caching Ensembl-RefSeq matches took: ", timer);
	timer.start();

	//init base gene infos
	NGSD db;
	db.approvedGeneNames();
	Log::perf("Caching gene names took: ", timer);
	timer.start();

	//init transcript infos
	db.transcripts();
	Log::perf("Caching transcripts took: ", timer);
	timer.start();

	//init phenotype infos
	db.phenotypes(QStringList());
	Log::perf("Caching phenotype took: ", timer);
	timer.start();

	//init relevant transcripts cache
	GSvarHelper::relevantTranscripts();
	Log::perf("Caching relevant transcripts took: ", timer);
}
