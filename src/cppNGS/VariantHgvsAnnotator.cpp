#include "VariantHgvsAnnotator.h"

VariantHgvsAnnotator::VariantHgvsAnnotator()
    : max_dist_to_transcript_(5000)
    , splice_region_ex_(3)
    , splice_region_in_(20)
{
}

VariantHgvsAnnotator::VariantHgvsAnnotator(int max_dist_to_transcript, int splice_region_ex, int splice_region_in)
    : max_dist_to_transcript_(max_dist_to_transcript)
    , splice_region_ex_(splice_region_ex)
    , splice_region_in_(splice_region_in)
{
}

HgvsNomenclature VariantHgvsAnnotator::variantToHgvs(Transcript& transcript, const VcfLine& variant, const FastaFileIndex& genome_idx)
{
    //init
    const Chromosome& chr = transcript.regions()[0].chr();
    int start = variant.start();
    int end = variant.end();

    //check prerequisites
    if (transcript.regions().count()==0) THROW(ProgrammingException, "Transcript '" + transcript.name() + "' has no regions() defined!");

    Sequence ref = variant.ref();
    Sequence obs = variant.alt().at(0);
    QString pos;

    HgvsNomenclature hgvs;
    hgvs.transcript_id = transcript.name();

    //determine position of variant w.r.t. transcript
    if(transcript.isCoding())
    {
        if(end < transcript.codingStart())
        {
            //in 5 prime utr or upstream variant?
            if((transcript.strand() == Transcript::PLUS && end >= transcript.utr5prime()[0].start()) ||
                    (transcript.strand() == Transcript::MINUS && start <= transcript.utr5prime()[transcript.utr5prime().count()-1].end()))
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT);
            }
            else if(transcript.utr5prime()[0].start() - end >= max_dist_to_transcript_)
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::UPSTREAM_GENE_VARIANT);
                return hgvs;
            }
            else
            {
                THROW(Exception, "Variant is too far upstream of transcript!");
            }
        }
        else if(start > transcript.codingEnd())
        {
            //in 3 prime utr or downstream variant?
            if((transcript.strand() == Transcript::PLUS && start <= transcript.utr3prime()[transcript.utr3prime().count()-1].end()) ||
                    (transcript.strand() == Transcript::MINUS && end >= transcript.utr3prime()[0].start()))
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::THREE_PRIME_UTR_VARIANT);
            }
            else if(start - transcript.utr3prime()[0].end() >= max_dist_to_transcript_)
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT);
                return hgvs;
            }
        }
    }

    return hgvs;
}

HgvsNomenclature VariantHgvsAnnotator::variantToHgvs(Transcript& transcript, const Variant& variant, const FastaFileIndex& genome_idx)
{
    HgvsNomenclature hgvs;
    return hgvs;
}
