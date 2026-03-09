#ifndef GENOMEDATA_H
#define GENOMEDATA_H

#include "cppVISUAL_global.h"
#include "Transcript.h"
#include "FastaFileIndex.h"
#include "Settings.h"
#include "ChromosomalIndex.h"

//Genome data for visualization
struct CPPVISUALSHARED_EXPORT GenomeData
{
    GenomeData()
        : genome_index(Settings::string("reference_genome", false))
        , transcripts()
        , transcript_index(transcripts)
    {
    }

    FastaFileIndex genome_index;

    TranscriptList transcripts;
    ChromosomalIndex<TranscriptList> transcript_index;
};

#endif // GENOMEDATA_H
