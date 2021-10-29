#include "VariantHgvsAnnotator.h"

VariantHgvsAnnotator::VariantHgvsAnnotator()
    : VariantHgvsAnnotator::VariantHgvsAnnotator(5000, 3, 8)
{
}

VariantHgvsAnnotator::VariantHgvsAnnotator(int max_dist_to_transcript, int splice_region_ex, int splice_region_in)
    : max_dist_to_transcript_(max_dist_to_transcript)
    , splice_region_ex_(splice_region_ex)
    , splice_region_in_(splice_region_in)
{
}

//convert a variant in VCF format into an HgvsNomenclature object
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
                pos_hgvs_c = getHgvsPosition(transcript.utr5prime(), variant, plus_strand, true);

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
                pos_hgvs_c = "*" + getHgvsPosition(transcript.utr3prime(), variant, plus_strand, false);
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
            pos_hgvs_c = getHgvsPosition(transcript.codingRegions(), variant, plus_strand, false);

            if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
            }
            else if(variant.isSNV())
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::CODING_SEQUENCE_VARIANT);
                hgvs.hgvs_p = getHgvsProteinAnnotation(variant, genome_idx, pos_hgvs_c, plus_strand);

                //annotate effect on protein sequence
                annotateProtSeqCsqSnv(hgvs);
            }
        }
    }
    // non-coding transcript
    else
    {
        if((plus_strand && variant.start() >= transcript.start() && variant.end() <= transcript.end()) ||
                (!plus_strand && variant.start() >= transcript.end() && variant.end() <= transcript.start()))
        {
            hgvs.variant_consequence_type.append(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT);

            pos_hgvs_c = getHgvsPosition(transcript.regions(), variant, plus_strand, false);

            if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::INTRON_VARIANT);
            }
            else
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT);
            }
        }
        else
        {
            THROW(Exception, "For non-coding transcripts, only variants within the transcript can be described.");
        }
    }

    //find out if the variant is a splice region variant
    annotateSpliceRegion(hgvs, transcript, start, end, plus_strand);

    QString hgvs_c_prefix = transcript.isCoding() ? "c." : "n.";

    //SNV
    if(variant.isSNV())
    {
        if(plus_strand)
        {
            hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + ref + ">" + obs;
        }
        else
        {
            hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + ref.toReverseComplement() + ">" + obs.toReverseComplement();
        }
    }

    return hgvs;
}

//convert a variant in GSvar format into an HgvsNomenclature object
HgvsNomenclature VariantHgvsAnnotator::variantToHgvs(const Transcript& transcript, const Variant& variant, const FastaFileIndex& genome_idx)
{
    //first convert from Variant to VcfLine
    VariantVcfRepresentation vcf_rep = variant.toVCF(genome_idx);
    QVector<Sequence> alt;
    alt.push_back(vcf_rep.alt);
    VcfLine vcf_variant(vcf_rep.chr, vcf_rep.pos, vcf_rep.ref, alt);

    return variantToHgvs(transcript, vcf_variant, genome_idx);
}

//determine the HGVS position string for a variant in 5/3 prime utr or coding region
QString VariantHgvsAnnotator::getHgvsPosition(const BedFile& regions, const VcfLine& variant, bool plus_strand, bool utr_5)
{
    int start = variant.start();
    int end = variant.end();
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
QByteArray VariantHgvsAnnotator::translate(const Sequence& seq, bool is_mito)
{
    if(seq.length() % 3 != 0) THROW(ArgumentException, "Coding sequence length must be multiple of three.")

    QByteArray aa_seq;

    for(int i=0; i<seq.length(); i+=3)
    {
        aa_seq.append(toThreeLetterCode(NGSHelper::translateCodon(seq.mid(i, 3), is_mito)));
    }
    return aa_seq;
}

//determine the annotation of the variant according to the HGVS nomenclature for proteins
QString VariantHgvsAnnotator::getHgvsProteinAnnotation(const VcfLine& variant, const FastaFileIndex& genome_idx,
                                                       const QString& pos_hgvs_c, bool plus_strand)
{
    QString hgvs_p("p.");
    int pos_trans_start = 0;
    //int pos trans_end = 0;
    int start = variant.start();
    //int end = variant.end();
    QByteArray aa_ref;
    QByteArray aa_obs;
    Sequence seq_obs;

    if(variant.isSNV())
    {
        //make position in transcript sequence zero-based
        pos_trans_start = pos_hgvs_c.toInt() - 1;
        //pos_trans_end = pos_trans_start;
        int offset = pos_trans_start % 3;

        //translate the reference sequence codon and obtain the observed sequence codon
        if(plus_strand)
        {
            aa_ref = toThreeLetterCode(NGSHelper::translateCodon(genome_idx.seq(variant.chr(), start - offset, 3),
                                                                          variant.chr().isM()));
            seq_obs = genome_idx.seq(variant.chr(), start - offset, 3);
            seq_obs[offset] = variant.alt().at(0)[0];
        }
        else
        {
            aa_ref = toThreeLetterCode(NGSHelper::translateCodon(genome_idx.seq(variant.chr(), start + offset - 2, 3).toReverseComplement(),
                                                                          variant.chr().isM()));
            seq_obs = genome_idx.seq(variant.chr(), start + offset - 2, 3);
            seq_obs.reverse();
            seq_obs[offset] = variant.alt().at(0)[0];
            seq_obs.complement();
        }

        //translate the observed sequence codon
        aa_obs = toThreeLetterCode(NGSHelper::translateCodon(seq_obs, variant.chr().isM()));

        if(aa_obs == aa_ref) aa_obs = "=";
        else if(aa_ref == "Met") aa_obs = "?";

        aa_ref.append(QByteArray::number(pos_trans_start / 3 + 1));
    }

    hgvs_p.append(aa_ref);
    hgvs_p.append(aa_obs);

    return hgvs_p;
}

//add the consequence type according to the change in the protein sequence
void VariantHgvsAnnotator::annotateProtSeqCsqSnv(HgvsNomenclature& hgvs)
{
    if(hgvs.hgvs_p.endsWith("="))
    {
        hgvs.variant_consequence_type.append(VariantConsequenceType::SYNONYMOUS_VARIANT);
        if(hgvs.hgvs_p.contains("Ter"))
        {
            hgvs.variant_consequence_type.append(VariantConsequenceType::STOP_RETAINED_VARIANT);
        }
        else if(hgvs.hgvs_p.contains("Met"))
        {
            hgvs.variant_consequence_type.append(VariantConsequenceType::START_RETAINED_VARIANT);
        }
        return;
    }
    else
    {
        hgvs.variant_consequence_type.append(VariantConsequenceType::PROTEIN_ALTERING_VARIANT);
    }

    if(hgvs.hgvs_p.contains("Met") && hgvs.hgvs_p.endsWith("?"))
    {
        hgvs.variant_consequence_type.append(VariantConsequenceType::START_LOST);
    }
    else if(hgvs.hgvs_p.endsWith("Ter"))
    {
        hgvs.variant_consequence_type.append(VariantConsequenceType::STOP_GAINED);
    }
    else if(hgvs.hgvs_p.contains("Ter"))
    {
        hgvs.variant_consequence_type.append(VariantConsequenceType::STOP_LOST);
    }
    else
    {
        hgvs.variant_consequence_type.append(VariantConsequenceType::MISSENSE_VARIANT);
    }
}

//annotate if the variant is a splice region variant
void VariantHgvsAnnotator::annotateSpliceRegion(HgvsNomenclature& hgvs, const Transcript& transcript, int start, int end, bool plus_strand)
{
    for(int i=0; i<transcript.regions().count(); i++)
    {
        int diff_intron_end = transcript.regions()[i].start() - end;
        int diff_exon_start = start - transcript.regions()[i].start() + 1;
        int diff_exon_end = transcript.regions()[i].end() - end + 1;
        int diff_intron_start = start - transcript.regions()[i].end();

        if((diff_intron_end <= splice_region_in_ && diff_intron_end >= 0) ||
                (diff_exon_start <= splice_region_ex_ && diff_exon_start >= 0))
        {
            //first exon cannot have splice region variant at the start
            if(i != 0)
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::SPLICE_REGION_VARIANT);
                if(diff_intron_end <= 2 && diff_intron_end > 0)
                {
                    if(plus_strand) hgvs.variant_consequence_type.append(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT);
                    else hgvs.variant_consequence_type.append(VariantConsequenceType::SPLICE_DONOR_VARIANT);

                    //splice donor/acceptor variant has unknown consequences on protein
                    hgvs.hgvs_p = "p.?";
                }
                break;
            }
        }
        else if((diff_exon_end <= splice_region_ex_ && diff_exon_end >= 0) ||
                (diff_intron_start <= splice_region_in_ && diff_intron_start >= 0))
        {
            //last exon cannot have splice region variant at the end
            if(i != transcript.regions().count() - 1)
            {
                hgvs.variant_consequence_type.append(VariantConsequenceType::SPLICE_REGION_VARIANT);
                if(diff_intron_start <= 2 && diff_intron_start > 0)
                {
                    if(plus_strand) hgvs.variant_consequence_type.append(VariantConsequenceType::SPLICE_DONOR_VARIANT);
                    else hgvs.variant_consequence_type.append(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT);

                    //splice donor/acceptor variant has unknown consequences on protein
                    hgvs.hgvs_p = "p.?";
                }
                break;
            }
        }
    }
}

//transform one letter aa code to three letter code; replacing "*" with "Ter" to encode for termination codon
//  (needed for HGVS nomenclature to obtain same output as from VEP)
QByteArray VariantHgvsAnnotator::toThreeLetterCode(QChar aa_one_letter_code)
{
    if(aa_one_letter_code == "*") return "Ter";
    else return NGSHelper::threeLetterCode(aa_one_letter_code);
}
