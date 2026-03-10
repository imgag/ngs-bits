#ifndef GENOMEDATA_H
#define GENOMEDATA_H

#include "cppVISUAL_global.h"
#include "Transcript.h"
#include "FastaFileIndex.h"
#include "ChromosomalIndex.h"

//general genome data needed for visualization
class CPPVISUALSHARED_EXPORT GenomeData
{
public:
	//default constructor (loads only genome index)
    GenomeData();
	//sets transcripts
    void setTranscripts(const TranscriptList& transcipts);

    //Returns the genome index
    const FastaFileIndex& genome() const
    {
        return genome_index_;
    }

    //Return transcripts
    const TranscriptList& transcripts() const
    {
        return transcripts_;
    }

    //Return index of transcripts for fast access by chromosomal position
    const ChromosomalIndex<TranscriptList>& transcriptsIndex() const
    {
        return transcripts_index_;
    }

protected:
    FastaFileIndex genome_index_;
    TranscriptList transcripts_;
    ChromosomalIndex<TranscriptList> transcripts_index_;
};

#endif // GENOMEDATA_H
