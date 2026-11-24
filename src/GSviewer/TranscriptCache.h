#ifndef TRANSCRIPTCACHE_H
#define TRANSCRIPTCACHE_H

#include "Transcript.h"

//Loads GSviewer transcripts, caching parsed GFF data as one TSV row per transcript.
class TranscriptCache
{
public:
	//An empty cache directory selects AppLocalDataLocation/transcript_cache.
	static TranscriptList load(const QString& gff_file, const QString& cache_directory = QString());
};

#endif // TRANSCRIPTCACHE_H
