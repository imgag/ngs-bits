#include "GenomeData.h"
#include "Settings.h"

GenomeData::GenomeData()
    : genome_index_(Settings::string("reference_genome", false))
    , transcripts_()
    , transcripts_index_(transcripts_)
{
}

void GenomeData::setTranscripts(const TranscriptList& transcipts)
{
    for(const Transcript& t: transcipts)
    {
        if (t.source()!=Transcript::ENSEMBL) continue;
        transcripts_ << t;
    }

    //sort and index transcripts
    if (!transcripts_.isSorted()) transcripts_.sortByPosition();
    transcripts_index_.createIndex();
}

