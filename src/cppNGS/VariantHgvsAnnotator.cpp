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

HgvsNomenclature VariantHgvsAnnotator::variantToHgvs(const Transcript& transcript, const VcfLine& variant, const FastaFileIndex& genome_idx)
{
    //init
    int start = variant.start();
    int end = variant.end();
    bool plus_strand = transcript.strand() == Transcript::PLUS;

    //check prerequisites
    if (transcript.regions().count()==0) THROW(ProgrammingException, "Transcript '" + transcript.name() + "' has no regions() defined!");

    Sequence ref = variant.ref();
    Sequence obs = variant.alt().at(0);
    QString pos_hgvs_c;

    HgvsNomenclature hgvs;
    hgvs.transcript_id = transcript.name();

    //determine position of variant w.r.t. transcript
    if(transcript.isCoding())
    {
        //upstream of start codon
        if((plus_strand && end < transcript.codingStart()) ||
                (!plus_strand && start > transcript.codingStart()))
        {
            //in 5 prime utr or upstream variant?
            if((plus_strand && end >= transcript.start()) ||
                    (!plus_strand && start <= transcript.end()))
            {
                bool five_prime_utr = false;
                int utr_pos = 0;
                //check for overlap with 5 prime utr exons
                for(int i=0; i<transcript.utr5prime().count(); i++)
                {
                    if(transcript.utr5prime()[i].overlapsWith(start, end))
                    {
                        hgvs.variant_consequence_type.append(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT);
                        five_prime_utr = true;
                        if(plus_strand)
                        {
                            utr_pos = transcript.utr5prime()[i].end() - start + 1;
                            continue;
                        }
                        else
                        {
                            utr_pos += end - transcript.utr5prime()[i].start() + 1;
                            break;
                        }
                    }
                    if((plus_strand && five_prime_utr) || !plus_strand)
                    {
                        utr_pos += transcript.utr5prime()[i].length();
                    }
                }
                if(five_prime_utr && variant.isSNV())
                {
                    pos_hgvs_c = "-" + QString::number(utr_pos);
                }
                else if(!five_prime_utr)
                {
                    hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
                    if(variant.isSNV())
                    {
                        pos_hgvs_c = "-" + getPositionInIntron(transcript.utr5prime(), variant.start(), plus_strand, true);
                    }
                }
            }
            else if((plus_strand && transcript.start() - end <= max_dist_to_transcript_) ||
                    (!plus_strand && start - transcript.end() <= max_dist_to_transcript_))
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::UPSTREAM_GENE_VARIANT);
                return hgvs;
            }
            else
            {
                THROW(Exception, "Variant is too far upstream of transcript!");
            }
        }
        //downstream of stop codon
        else if((plus_strand && start > transcript.codingEnd()) ||
                (!plus_strand && end < transcript.codingEnd()))
        {
            //in 3 prime utr or downstream variant?
            if((plus_strand && start <= transcript.end()) ||
                    (!plus_strand && end >= transcript.start()))
            {
                bool three_prime_utr = false;
                int utr_pos = 0;
                //check for overlap with 3 prime utr exons
                for(int i=0; i<transcript.utr3prime().count(); i++)
                {
                    if(transcript.utr3prime()[i].overlapsWith(start, end))
                    {
                        hgvs.variant_consequence_type.append(VariantConsequenceType::THREE_PRIME_UTR_VARIANT);
                        three_prime_utr = true;
                        if(plus_strand)
                        {
                            utr_pos += start - transcript.utr3prime()[i].start() + 1;
                            break;
                        }
                        else
                        {
                            utr_pos = transcript.utr3prime()[i].end() - end + 1;
                            continue;
                        }
                    }
                    if(plus_strand || (!plus_strand && three_prime_utr))
                    {
                        utr_pos += transcript.utr3prime()[i].length();
                    }
                }
                if(three_prime_utr && variant.isSNV())
                {
                    pos_hgvs_c = "*" + QString::number(utr_pos);
                }
                else if(!three_prime_utr)
                {
                    hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
                    if(variant.isSNV())
                    {
                        pos_hgvs_c = "*" + getPositionInIntron(transcript.utr3prime(), variant.start(), plus_strand);
                    }
                }
            }
            else if((plus_strand && start - transcript.end() <= max_dist_to_transcript_) ||
                    (!plus_strand && transcript.start() - end <= max_dist_to_transcript_))
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT);
                return hgvs;
            }
            else
            {
                THROW(Exception, "Variant is too far downstream of transcript");
            }
        }
        //between start and stop codon
        else
        {
            bool exon = false;
            int coding_pos = 0;
            for(int i=0; i<transcript.codingRegions().count(); i++)
            {
                if(transcript.codingRegions()[i].overlapsWith(start, end))
                {
                    exon = true;
                    hgvs.variant_consequence_type.append(VariantConsequenceType::CODING_SEQUENCE_VARIANT);
                    if(plus_strand)
                    {
                        coding_pos += start - transcript.codingRegions()[i].start() + 1;
                        break;
                    }
                    else
                    {
                        coding_pos = transcript.codingRegions()[i].end() - end + 1;
                        continue;
                    }
                }
                if(plus_strand || (!plus_strand && exon))
                {
                    coding_pos += transcript.codingRegions()[i].length();
                }
            }
            if(exon && variant.isSNV())
            {
                pos_hgvs_c = QString::number(coding_pos);
            }
            else if(!exon)
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
                if(variant.isSNV())
                {
                    pos_hgvs_c = getPositionInIntron(transcript.codingRegions(), variant.start(), plus_strand);
                }
            }
        }
    }

    //SNV
    if(variant.isSNV())
    {
        if(plus_strand)
        {
            hgvs.hgvs_c = "c." + pos_hgvs_c + ref + ">" + obs;
        }
        else
        {
            hgvs.hgvs_c = "c." + pos_hgvs_c + ref.toReverseComplement() + ">" + obs.toReverseComplement();
        }
    }

    return hgvs;
}

HgvsNomenclature VariantHgvsAnnotator::variantToHgvs(const Transcript& transcript, const Variant& variant, const FastaFileIndex& genome_idx)
{
    HgvsNomenclature hgvs;
    return hgvs;
}

//determine the HGVS position string for a variant in an intron
QString VariantHgvsAnnotator::getPositionInIntron(const BedFile& regions, int genomic_position, bool plus_strand, bool utr_5)
{
    QString pos_in_intron;
    int closest_exon_pos = 0;
    bool pos_found = false;

    //turn strand info around if 5 prime utr introns are considered (counting starts at opposite end)
    if(utr_5) plus_strand = !plus_strand;

    for(int i=0; i<regions.count()-1; i++)
    {
        if(plus_strand)
        {
            closest_exon_pos += regions[i].length();
        }
        else if(pos_found)
        {
            closest_exon_pos += regions[i+1].length();
        }

        //check if genomic position is between two given exons
        if(genomic_position > regions[i].end()
                && genomic_position < regions[i+1].start())
        {
            pos_found = true;
            int prev_exon_end = regions[i].end();
            int next_exon_start = regions[i+1].start();

            int dist_below = genomic_position - prev_exon_end;
            int dist_above = next_exon_start - genomic_position;

            if(plus_strand)
            {
                if(dist_below <= dist_above)
                {
                    QString prefix = utr_5 ? "-" : "+";
                    pos_in_intron = prefix + QString::number(dist_below);
                }
                else
                {
                    QString prefix = utr_5 ? "+" : "-";
                    pos_in_intron = prefix + QString::number(dist_above);
                }
                break;
            }
            else
            {
                closest_exon_pos += regions[i+1].length();
                if(dist_above <= dist_below)
                {                    
                    QString prefix = utr_5 ? "-" : "+";
                    pos_in_intron = prefix + QString::number(dist_above);
                }
                else
                {
                    QString prefix = utr_5 ? "+" : "-";
                    pos_in_intron = prefix + QString::number(dist_below);
                }
            }
        }
    }

    if(pos_in_intron.startsWith("+"))
    {
        if(utr_5)
        {
            pos_in_intron = QString::number(closest_exon_pos+1) + pos_in_intron;
        }
        else
        {
            pos_in_intron = QString::number(closest_exon_pos) + pos_in_intron;
        }
    }
    else if(pos_in_intron.startsWith("-"))
    {
        if(utr_5)
        {
            pos_in_intron = QString::number(closest_exon_pos) + pos_in_intron;
        }
        else
        {
            pos_in_intron = QString::number(closest_exon_pos+1) + pos_in_intron;
        }
    }

    return pos_in_intron;
}
