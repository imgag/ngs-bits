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
HgvsNomenclature VariantHgvsAnnotator::variantToHgvs(const Transcript& transcript, VcfLine& variant, const FastaFileIndex& genome_idx)
{
    //init
    bool plus_strand = transcript.strand() == Transcript::PLUS;
    VcfLine::ShiftDirection shift_dir = plus_strand ? VcfLine::ShiftDirection::RIGHT : VcfLine::ShiftDirection::LEFT;
    variant.normalize(shift_dir, genome_idx);
    int start = variant.start();
    int end = variant.end();

    //check prerequisites
    if (transcript.regions().count()==0) THROW(ProgrammingException, "Transcript '" + transcript.name() + "' has no regions() defined!");

    Sequence ref = variant.ref();
    Sequence obs = variant.alt().at(0);
    QString pos_hgvs_c;

    HgvsNomenclature hgvs;
    hgvs.transcript_id = transcript.name();

    // annotate coding transcript
    if(transcript.isCoding())
    {
        if(variant.isSNV())
        {
            pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start, plus_strand);
        }
        //deletion
        else if(variant.isDel())
        {
            pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start + 1, plus_strand);

            if(end - start > 1)
            {
                if(plus_strand)
                {
                    pos_hgvs_c += "_" + annotateRegionsCoding(transcript, hgvs, end, plus_strand);
                }
                else
                {
                    pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, end, plus_strand) + "_" + pos_hgvs_c;
                }
            }
        }
        //insertion
        else if(variant.isIns())
        {
            if(plus_strand)
            {
                pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start, plus_strand) + "_" +
                        annotateRegionsCoding(transcript, hgvs, start + 1, plus_strand);
            }
            else
            {
                pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start + 1, plus_strand) + "_" +
                        annotateRegionsCoding(transcript, hgvs, start, plus_strand);
            }
        }
        //delins (deletion and insertion at the same time/substitution of more than one base)
        else if(variant.isInDel() && !variant.isDel() && !variant.isIns())
        {
            //normalization made sure that alt and ref always have the same base prepended
            pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start + 1, plus_strand);

            if(end - start > 1)
            {
                if(plus_strand)
                {
                    pos_hgvs_c += "_" + annotateRegionsCoding(transcript, hgvs, end, plus_strand);
                }
                else
                {
                    pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, end, plus_strand) + "_" + pos_hgvs_c;
                }
            }
        }

        // up- or downstream variant, no description w.r.t. cDNA positions possible
        if(pos_hgvs_c == "") return hgvs;

        // create HGVS protein annotation
        if(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT))
        {
            hgvs.hgvs_p = getHgvsProteinAnnotation(variant, genome_idx, pos_hgvs_c, plus_strand);
        }
    }
    // non-coding transcript
    else
    {
        if(variant.isSNV())
        {
            pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start, plus_strand);
        }
        else if(variant.isDel())
        {
            pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start + 1, plus_strand);

            if(end - start > 1)
            {
                if(plus_strand)
                {
                    pos_hgvs_c += "_" + annotateRegionsNonCoding(transcript, hgvs, end, plus_strand);
                }
                else
                {
                    pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, end, plus_strand) + "_" + pos_hgvs_c;
                }
            }
        }
        else if(variant.isIns())
        {
            if(plus_strand)
            {
                pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start, plus_strand) + "_" +
                        annotateRegionsNonCoding(transcript, hgvs, start + 1, plus_strand);
            }
            else
            {
                pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start + 1, plus_strand) + "_" +
                        annotateRegionsNonCoding(transcript, hgvs, start, plus_strand);
            }
        }
        else if(variant.isInDel() && !variant.isDel() && !variant.isIns())
        {
            //normalization made sure that alt and ref always have the same base prepended
            pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start + 1, plus_strand);

            if(end - start > 1)
            {
                if(plus_strand)
                {
                    pos_hgvs_c += "_" + annotateRegionsNonCoding(transcript, hgvs, end, plus_strand);
                }
                else
                {
                    pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, end, plus_strand) + "_" + pos_hgvs_c;
                }
            }
        }
    }

    //find out if the variant is a splice region variant
    annotateSpliceRegion(hgvs, transcript, start, end, plus_strand);

    QString hgvs_c_prefix = transcript.isCoding() ? "c." : "n.";

    //SNV
    if(variant.isSNV())
    {
        if(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT))
        {
            //annotate effect on protein sequence
            annotateProtSeqCsqSnv(hgvs);
        }

        if(plus_strand)
        {
            hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + ref + ">" + obs;
        }
        else
        {
            hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + ref.toReverseComplement() + ">" + obs.toReverseComplement();
        }
    }
    //deletion
    else if(variant.isDel())
    {
        hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + "del";
    }
    //insertion
    else if(variant.isIns())
    {
        Sequence variant_alt_seq = variant.alt()[0];
        variant_alt_seq = variant_alt_seq.right(variant_alt_seq.length() - 1);
        if(!plus_strand) variant_alt_seq.reverseComplement();
        hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + "ins" + variant_alt_seq;
    }
    //delins (deletion and insertion at the same time/substitution of more than one base)
    else if(variant.isInDel() && !variant.isDel() && !variant.isIns())
    {
        Sequence alt = variant.alt(0).mid(1);
        if(!plus_strand) alt.reverseComplement();
        hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + "delins" + alt;
    }

    if(!variant.isSNV() && hgvs.hgvs_p != "")
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::PROTEIN_ALTERING_VARIANT);

        if(hgvs.hgvs_p.contains("fs"))
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::FRAMESHIFT_VARIANT);
        }
        else if(variant.isDel())
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::INFRAME_DELETION);
        }
        else if(variant.isIns())
        {
            if(hgvs.hgvs_p.endsWith("Ter"))
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::STOP_GAINED);
            }
            else
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::INFRAME_INSERTION);
            }
        }
        else
        {
            if(variant.ref().length() == variant.alt(0).length())
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::MISSENSE_VARIANT);
            }
            else if(variant.ref().length() > variant.alt(0).length())
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::INFRAME_DELETION);
            }
            else
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::INFRAME_INSERTION);
            }
        }
    }

    return hgvs;
}

//convert a variant in GSvar format into an HgvsNomenclature object
HgvsNomenclature VariantHgvsAnnotator::variantToHgvs(const Transcript& transcript, Variant &variant, const FastaFileIndex& genome_idx)
{
    //first convert from Variant to VcfLine
    VariantVcfRepresentation vcf_rep = variant.toVCF(genome_idx);
    QVector<Sequence> alt;
    alt.push_back(vcf_rep.alt);
    VcfLine vcf_variant(vcf_rep.chr, vcf_rep.pos, vcf_rep.ref, alt);

    return variantToHgvs(transcript, vcf_variant, genome_idx);
}

// make variant consequence type annotations depending on the part of the transcript the variant occurs in;
// for coding variants and a single genomic position
QString VariantHgvsAnnotator::annotateRegionsCoding(const Transcript& transcript, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand)
{
    QString pos_hgvs_c;

    //upstream of start codon
    if((plus_strand && gen_pos < transcript.codingStart()) ||
            (!plus_strand && gen_pos > transcript.codingStart()))
    {
        //in 5 prime utr or upstream variant?
        if((plus_strand && gen_pos >= transcript.start()) ||
                (!plus_strand && gen_pos <= transcript.end()))
        {
            pos_hgvs_c = getHgvsPosition(transcript.utr5prime(), hgvs, gen_pos, plus_strand, true);

            if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::INTRON_VARIANT);
            }
            else
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT);
            }
            pos_hgvs_c = "-" + pos_hgvs_c;
        }
        else if((plus_strand && transcript.start() - gen_pos <= max_dist_to_transcript_) ||
                (!plus_strand && gen_pos - transcript.end() <= max_dist_to_transcript_))
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::INTERGENIC_VARIANT);
            hgvs.variant_consequence_type.insert(VariantConsequenceType::UPSTREAM_GENE_VARIANT);
            return "";
        }
        else
        {
            THROW(Exception, "Variant is too far upstream of transcript!");
        }
    }
    //downstream of stop codon
    else if((plus_strand && gen_pos > transcript.codingEnd()) ||
            (!plus_strand && gen_pos < transcript.codingEnd()))
    {
        //in 3 prime utr or downstream variant?
        if((plus_strand && gen_pos <= transcript.end()) ||
                (!plus_strand && gen_pos >= transcript.start()))
        {
            //determine number of first exon in 3 prime utr; subtract 2 because of exons that are both utr and cds
            int first_region = transcript.utr5prime().count() + transcript.codingRegions().count() - 2;

            pos_hgvs_c = "*" + getHgvsPosition(transcript.utr3prime(), hgvs, gen_pos, plus_strand, false, first_region);

            if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::INTRON_VARIANT);
            }
            else
            {
                hgvs.variant_consequence_type.insert(VariantConsequenceType::THREE_PRIME_UTR_VARIANT);
            }
        }
        else if((plus_strand && gen_pos - transcript.end() <= max_dist_to_transcript_) ||
                (!plus_strand && transcript.start() - gen_pos <= max_dist_to_transcript_))
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::INTERGENIC_VARIANT);
            hgvs.variant_consequence_type.insert(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT);
            return "";
        }
        else
        {
            THROW(Exception, "Variant is too far downstream of transcript");
        }
    }
    //between start and stop codon
    else
    {
        //determine number of first exon in coding sequence; subtract 1 because of exon that is both utr and cds
        int first_region = transcript.utr5prime().count() - 1;

        pos_hgvs_c = getHgvsPosition(transcript.codingRegions(), hgvs, gen_pos, plus_strand, false, first_region);

        if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::INTRON_VARIANT);
        }
        else
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::CODING_SEQUENCE_VARIANT);
        }
    }
    return pos_hgvs_c;
}

// make variant consequence type annotations depending on the part of the transcript the variant occurs in;
// for non-coding variants and a single genomic position
QString VariantHgvsAnnotator::annotateRegionsNonCoding(const Transcript& transcript, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand)
{
    QString pos_hgvs_c;

    if((plus_strand && gen_pos >= transcript.start() && gen_pos <= transcript.end()) ||
            (!plus_strand && gen_pos >= transcript.end() && gen_pos <= transcript.start()))
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT);

        pos_hgvs_c = getHgvsPosition(transcript.regions(), hgvs, gen_pos, plus_strand, false);

        if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::INTRON_VARIANT);
        }
        else
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT);
        }
    }
    // variant downstream of non-coding transcript
    else if((plus_strand && gen_pos - transcript.end() <= max_dist_to_transcript_) ||
            (!plus_strand && transcript.start() - gen_pos <= max_dist_to_transcript_))
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::INTERGENIC_VARIANT);
        hgvs.variant_consequence_type.insert(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT);
        return "";
    }
    // variant upstream of non-coding transcript
    else if((plus_strand && transcript.start() - gen_pos <= max_dist_to_transcript_) ||
            (!plus_strand && gen_pos - transcript.end() <= max_dist_to_transcript_))
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::INTERGENIC_VARIANT);
        hgvs.variant_consequence_type.insert(VariantConsequenceType::UPSTREAM_GENE_VARIANT);
        return "";
    }
    else
    {
        THROW(Exception, "Variant is too far up/downstream of transcript");
    }
    return pos_hgvs_c;
}

//determine the HGVS position string for a single genomic position in any part of the transcript
QString VariantHgvsAnnotator::getHgvsPosition(const BedFile& regions, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand, bool utr_5, int first_region)
{
    bool in_exon = false;

    int pos = 0;

    QString pos_hgvs_c;

    //turn strand info around if 5 prime utr exons are considered (counting starts at opposite end)
    if(utr_5) plus_strand = !plus_strand;

    //check if genomic position is contained in exons
    for(int i=0; i<regions.count(); i++)
    {
        if(regions[i].overlapsWith(gen_pos, gen_pos))
        {
            in_exon = true;
            if(plus_strand)
            {
                pos += gen_pos - regions[i].start() + 1;

                if(utr_5) hgvs.exon_number = regions.count() - i + first_region;
                else hgvs.exon_number = i + 1 + first_region;
                break;
            }
            else
            {
                pos = regions[i].end() - gen_pos + 1;

                if(utr_5) hgvs.exon_number = i + 1 + first_region;
                else hgvs.exon_number = regions.count() - i + first_region;
                continue;
            }
        }
        if(plus_strand || (!plus_strand && in_exon))
        {
            pos += regions[i].length();
        }
    }
    if(in_exon)
    {
        pos_hgvs_c = QString::number(pos);
    }
    else
    {
        pos_hgvs_c = getPositionInIntron(regions, hgvs, gen_pos, plus_strand, utr_5, first_region);
    }
    return pos_hgvs_c;
}

//determine the HGVS position string for a single genomic position in an intron
QString VariantHgvsAnnotator::getPositionInIntron(const BedFile& regions, HgvsNomenclature& hgvs, int genomic_position, bool plus_strand, bool utr_5, int first_region)
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
                if(utr_5) hgvs.intron_number = regions.count() - i + first_region - 1;
                else hgvs.intron_number = i + 1 + first_region;

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
                if(utr_5) hgvs.intron_number = i + 1 + first_region;
                else hgvs.intron_number = regions.count() - i + first_region - 1;

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
    int start = variant.start();
    int end = variant.end();
    QByteArray aa_ref;
    QByteArray aa_obs;
    Sequence seq_obs;

    if(variant.isSNV())
    {
        //make position in transcript sequence zero-based
        pos_trans_start = pos_hgvs_c.toInt() - 1;
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
    else if(variant.isInDel())
    {
        //make position of start of deletion/insertion in transcript zero-based
        pos_trans_start = pos_hgvs_c.split("_").at(0).toInt() - 1;

        int offset = pos_trans_start % 3;
        int frame_diff = variant.isDel() ? end - start : variant.alt()[0].length() - variant.ref().length();
        int pos_shift = 0;

        // get a sufficiently large part of the reference and observed sequence
        int seq_length = 15;
        Sequence seq_ref;

        if(plus_strand)
        {
            if(variant.isDel())
            {
                seq_ref = genome_idx.seq(variant.chr(), start + 1 - offset, frame_diff + seq_length);
                seq_obs = seq_ref.left(offset) + seq_ref.right(seq_length - offset);
            }
            else if(variant.isIns())
            {
                seq_ref = genome_idx.seq(variant.chr(), start - offset, seq_length);
                seq_obs = seq_ref.left(offset + 1) + variant.alt(0).right(frame_diff) + seq_ref.right(seq_length - offset - 1);
            }
            else
            {
                seq_ref = genome_idx.seq(variant.chr(), start + 1 - offset, variant.ref().length() + seq_length - 1);
                seq_obs = seq_ref.left(offset + 1) + variant.alt(0).mid(1) + seq_ref.right(seq_length - offset);
            }
        }
        else
        {
            if(variant.isDel())
            {
                seq_ref = genome_idx.seq(variant.chr(), start - seq_length, frame_diff + seq_length + offset + 1);
                seq_obs = seq_ref.left(seq_length + 1) + seq_ref.right(offset);
            }
            else if(variant.isIns())
            {
                seq_ref = genome_idx.seq(variant.chr(), start - seq_length, seq_length + offset + 2);
                seq_obs = seq_ref.left(seq_length + 1) + variant.alt()[0].right(frame_diff) + seq_ref.right(offset + 1);
            }
            else
            {
                seq_ref = genome_idx.seq(variant.chr(), start - seq_length, variant.ref().length() + seq_length + offset);
                seq_obs = seq_ref.left(seq_length + 1) + variant.alt(0).mid(1) + seq_ref.right(offset);
            }
            seq_obs.reverseComplement();
            seq_ref.reverseComplement();
        }

        if(variant.isDel() || (variant.isIns() && frame_diff % 3 != 0) || (!variant.isIns() && !variant.isDel()))
        {
            //find the first amino acid that is changed due to the deletion/frameshift insertion/deletion-insertion
            while(aa_obs == aa_ref)
            {
                aa_ref = toThreeLetterCode(NGSHelper::translateCodon(seq_ref.left(3), variant.chr().isM()));
                aa_obs = toThreeLetterCode(NGSHelper::translateCodon(seq_obs.left(3), variant.chr().isM()));

                if(aa_obs == aa_ref)
                {
                    seq_obs = seq_obs.right(seq_obs.length() - 3);
                    seq_ref = seq_ref.right(seq_ref.length() - 3);
                    pos_shift += 3;
                }
            }
        }
        else if(variant.isIns())
        {
            aa_ref = toThreeLetterCode(NGSHelper::translateCodon(seq_ref.left(3), variant.chr().isM()));
            aa_obs = toThreeLetterCode(NGSHelper::translateCodon(seq_obs.left(3), variant.chr().isM()));

            QByteArray aa_ref_after = toThreeLetterCode(NGSHelper::translateCodon(seq_ref.mid(3, 3), variant.chr().isM()));
            QByteArray aa_obs_after = toThreeLetterCode(NGSHelper::translateCodon(seq_obs.mid(3 + frame_diff, 3), variant.chr().isM()));

            if(aa_obs == aa_ref && aa_obs_after == aa_ref_after)
            {
                aa_obs = "ins" + translate(seq_obs.mid(3, frame_diff));
            }
            //delins if first amino acid is changed by the insertion
            else if(aa_obs_after == aa_ref_after)
            {
                aa_obs = "delins" + translate(seq_obs.left(3 + frame_diff));
            }
        }
        aa_ref.append(QByteArray::number((pos_trans_start + pos_shift) / 3 + 1));

        // frameshift deletion/insertion/deletion-insertion
        if(frame_diff % 3 != 0)
        {
            //special case if first changed amino acid is changed into a stop codon: describe as substitution
            if(aa_obs != "Ter")
            {
               aa_obs = "fs";
            }
        }
        // inframe deletion
        else if(variant.isDel())
        {
            // more than one amino acid deleted
            if(frame_diff > 3)
            {
                int offset_end = (offset + 2) % 3;
                aa_ref.append("_");
                if(plus_strand)
                {
                    aa_ref.append(toThreeLetterCode(NGSHelper::translateCodon(genome_idx.seq(variant.chr(), end - offset_end, 3),
                                                                              variant.chr().isM())));
                }
                else
                {
                    aa_ref.append(toThreeLetterCode(NGSHelper::translateCodon(genome_idx.seq(variant.chr(), start - 2 + offset, 3).toReverseComplement(),
                                                                              variant.chr().isM())));
                }
                aa_ref.append(QByteArray::number((pos_trans_start + frame_diff) / 3 + 1));
            }

            // delin if first mismatched amino acid is not the first one after the deletion
            if(aa_obs != toThreeLetterCode(NGSHelper::translateCodon(seq_ref.mid(frame_diff, 3))))
            {
                aa_obs = "delins" + aa_obs;
            }
            else
            {
                aa_obs = "del";
            }
        }
        //inframe insertion
        else if(variant.isIns())
        {
            if(!aa_obs.startsWith("delins"))
            {
                aa_ref += "_" + toThreeLetterCode(NGSHelper::translateCodon(seq_ref.mid(3, 3)))
                        + QByteArray::number((pos_trans_start + pos_shift) / 3 + 2);
            }
        }
        //inframe deletion-insertion, more than one amino acid deleted
        else if(variant.ref().length() > (4 + pos_shift))
        {
            int offset_end = (offset + variant.ref().length() - 1) % 3;
            aa_ref.append("_");
            if(plus_strand)
            {
                aa_ref.append(toThreeLetterCode(NGSHelper::translateCodon(genome_idx.seq(variant.chr(), end - offset_end, 3),
                                                                          variant.chr().isM())));
            }
            else
            {
                aa_ref.append(toThreeLetterCode(NGSHelper::translateCodon(genome_idx.seq(variant.chr(), start - 2 + offset_end, 3).toReverseComplement(),
                                                                          variant.chr().isM())));
            }
            aa_ref.append(QByteArray::number((pos_trans_start + variant.ref().length() - pos_shift - 1) / 3 + 1));

            //more than one amino acid inserted
            if(variant.alt(0).length() > (4 + pos_shift))
            {
                aa_obs = "delins" + translate(seq_obs.left(((variant.alt(0).length() - 1) * 3) / 3 + 1  - pos_shift));
            }
            else
            {
                aa_obs = "delins" + aa_obs;
            }
        }
        //inframe deletion-insertion, more than one amino acid inserted
        else if(variant.alt(0).length() > (4 + pos_shift))
        {
            aa_obs = "delins" + translate(seq_obs.left(((variant.alt(0).length() - 1) * 3) / 3 + 1  - pos_shift));
        }
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
        hgvs.variant_consequence_type.insert(VariantConsequenceType::SYNONYMOUS_VARIANT);
        if(hgvs.hgvs_p.contains("Ter"))
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::STOP_RETAINED_VARIANT);
        }
        else if(hgvs.hgvs_p.contains("Met"))
        {
            hgvs.variant_consequence_type.insert(VariantConsequenceType::START_RETAINED_VARIANT);
        }
        return;
    }
    else
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::PROTEIN_ALTERING_VARIANT);
    }

    if(hgvs.hgvs_p.contains("Met") && hgvs.hgvs_p.endsWith("?"))
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::START_LOST);
    }
    else if(hgvs.hgvs_p.endsWith("Ter"))
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::STOP_GAINED);
    }
    else if(hgvs.hgvs_p.contains("Ter"))
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::STOP_LOST);
    }
    else
    {
        hgvs.variant_consequence_type.insert(VariantConsequenceType::MISSENSE_VARIANT);
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
                hgvs.variant_consequence_type.insert(VariantConsequenceType::SPLICE_REGION_VARIANT);
                if(diff_intron_end <= 2 && diff_intron_end > 0)
                {
                    if(plus_strand) hgvs.variant_consequence_type.insert(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT);
                    else hgvs.variant_consequence_type.insert(VariantConsequenceType::SPLICE_DONOR_VARIANT);

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
                hgvs.variant_consequence_type.insert(VariantConsequenceType::SPLICE_REGION_VARIANT);
                if(diff_intron_start <= 2 && diff_intron_start > 0)
                {
                    if(plus_strand) hgvs.variant_consequence_type.insert(VariantConsequenceType::SPLICE_DONOR_VARIANT);
                    else hgvs.variant_consequence_type.insert(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT);

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