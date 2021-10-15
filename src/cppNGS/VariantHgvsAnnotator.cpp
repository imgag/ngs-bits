#include "VariantHgvsAnnotator.h"

VariantHgvsAnnotator::VariantHgvsAnnotator()
    : VariantHgvsAnnotator::VariantHgvsAnnotator(5000, 3, 20)
{
}

VariantHgvsAnnotator::VariantHgvsAnnotator(int max_dist_to_transcript, int splice_region_ex, int splice_region_in)
    : max_dist_to_transcript_(max_dist_to_transcript)
    , splice_region_ex_(splice_region_ex)
    , splice_region_in_(splice_region_in)
    , code_sun_{{"TTT", "Phe"}, {"TTC", "Phe"}, {"TTA", "Leu"}, {"TTG", "Leu"}, {"CTT", "Leu"}, {"CTC", "Leu"},
                {"CTA", "Leu"}, {"CTG", "Leu"}, {"TCT", "Ser"}, {"TCC", "Ser"}, {"TCA", "Ser"}, {"TCG", "Ser"},
                {"AGT", "Ser"}, {"AGC", "Ser"}, {"TAT", "Tyr"}, {"TAC", "Tyr"}, {"TAA", "Ter"}, {"TAG", "Ter"},
                {"TGA", "Ter"}, {"TGT", "Cys"}, {"TGC", "Cys"}, {"TGG", "Trp"}, {"CCT", "Pro"}, {"CCC", "Pro"},
                {"CCA", "Pro"}, {"CCG", "Pro"}, {"CAT", "His"}, {"CAC", "His"}, {"CAA", "Gln"}, {"CAG", "Gln"},
                {"CGT", "Arg"}, {"CGC", "Arg"}, {"CGA", "Arg"}, {"CGG", "Arg"}, {"AGA", "Arg"}, {"AGG", "Arg"},
                {"ATT", "Ile"}, {"ATC", "Ile"}, {"ATA", "Ile"}, {"ATG", "Met"}, {"ACT", "Thr"}, {"ACC", "Thr"},
                {"ACA", "Thr"}, {"ACG", "Thr"}, {"AAT", "Asn"}, {"AAC", "Asn"}, {"AAA", "Lys"}, {"AAG", "Lys"},
                {"GTT", "Val"}, {"GTC", "Val"}, {"GTA", "Val"}, {"GTG", "Val"}, {"GCT", "Ala"}, {"GCC", "Ala"},
                {"GCA", "Ala"}, {"GCG", "Ala"}, {"GAT", "Asp"}, {"GAC", "Asp"}, {"GAA", "Glu"}, {"GAG", "Glu"},
                {"GGT", "Gly"}, {"GGC", "Gly"}, {"GGA", "Gly"}, {"GGG", "Gly"}}
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
                pos_hgvs_c = getHgvsPosition(transcript.utr5prime(), variant, end, start, plus_strand, true);

                if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
                {
                    hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
                }
                else
                {
                    hgvs.variant_consequence_type.append(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT);
                }
                pos_hgvs_c = "-" + pos_hgvs_c;
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
                pos_hgvs_c = "*" + getHgvsPosition(transcript.utr3prime(), variant, start, end, plus_strand, false);
                if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
                {
                    hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
                }
                else
                {
                    hgvs.variant_consequence_type.append(VariantConsequenceType::THREE_PRIME_UTR_VARIANT);
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
            pos_hgvs_c = getHgvsPosition(transcript.codingRegions(), variant, start, end, plus_strand, false);

            if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
            }
            else
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::CODING_SEQUENCE_VARIANT);
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

//determine the HGVS position string for a variant in the untranslated region
QString VariantHgvsAnnotator::getHgvsPosition(const BedFile& regions, const VcfLine& variant, int start, int end, bool plus_strand, bool utr_5)
{
    bool exon = false;
    int pos = 0;
    QString pos_hgvs_c;

    //turn strand info around if 5 prime utr introns are considered (counting starts at opposite end)
    if(utr_5) plus_strand = !plus_strand;

    //check for overlap with exons
    for(int i=0; i<regions.count(); i++)
    {
        if(regions[i].overlapsWith(start, end))
        {
            exon = true;
            if(plus_strand)
            {
                pos += start - regions[i].start() + 1;
                break;
            }
            else
            {
                pos = regions[i].end() - end + 1;
                continue;
            }
        }
        if(plus_strand || (!plus_strand && exon))
        {
            pos += regions[i].length();
        }
    }
    if(exon && variant.isSNV())
    {
        pos_hgvs_c = QString::number(pos);
    }
    else if(!exon && variant.isSNV())
    {
        pos_hgvs_c = getPositionInIntron(regions, start, plus_strand, utr_5);
    }
    return pos_hgvs_c;
}

//determine the HGVS position string for a variant in an intron
QString VariantHgvsAnnotator::getPositionInIntron(const BedFile& regions, int genomic_position, bool plus_strand, bool utr_5)
{
    QString pos_in_intron;
    int closest_exon_pos = 0;
    bool pos_found = false;

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

    //annotate position according to closest exon
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

//extract the coding DNA sequence of the transcript from the reference genome
Sequence VariantHgvsAnnotator::getCodingSequence(const Transcript& trans, const FastaFileIndex& genome_idx)
{
    Sequence seq;

    for(int i=0; i<trans.codingRegions().count(); i++)
    {
        int start = trans.codingRegions()[i].start();
        int length = trans.codingRegions()[i].end() - start + 1;
        seq.append(genome_idx.seq(trans.chr(), start, length));
    }

    if(trans.strand() == Transcript::MINUS)
    {
        seq.reverseComplement();
    }

    return seq;
}

//translate a DNA sequence into an amino acid sequence
QByteArray VariantHgvsAnnotator::translate(const Sequence& seq)
{
    if(seq.length() % 3 != 0) THROW(ArgumentException, "Coding sequence length must be multiple of three.")

    QByteArray aa_seq;

    for(int i=0; i<seq.length(); i+=3)
    {
        aa_seq.append(codonToAminoAcid(seq.mid(i, 3)));
    }
    return aa_seq;
}

//translate a single codon into its three-letter amino acid equivalent
QByteArray VariantHgvsAnnotator::codonToAminoAcid(const QByteArray& codon)
{
    if(codon.length() != 3) THROW(ArgumentException, "Codon must consist of exactly three bases.");

    if(code_sun_.contains(codon))
    {
        return code_sun_[codon];
    }
    else
    {
        THROW(ArgumentException, "Invalid codon");
    }
}
