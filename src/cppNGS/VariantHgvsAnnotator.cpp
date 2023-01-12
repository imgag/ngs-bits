#include "VariantHgvsAnnotator.h"

VariantHgvsAnnotator::VariantHgvsAnnotator(const FastaFileIndex& genome_idx, Parameters params)
	: params_(params)
	, genome_idx_(genome_idx)
{
}

//convert a variant in VCF format into an HgvsNomenclature object
VariantConsequence VariantHgvsAnnotator::annotate(const Transcript& transcript, const VcfLine& variant_orig, bool debug)
{
	VcfLine variant = variant_orig; //make a copy because we normalize that variant

	//check prerequisites
	if (transcript.regions().count()==0) THROW(ProgrammingException, "Cannot annotate consequences for ranscripts without regions: " + transcript.name());
	if (variant.isMultiAllelic()) THROW(ProgrammingException, "Cannot annotate consequences for multi-allelic variants: " + variant.toString());
	if (!variant.isValid()) THROW(ProgrammingException, "Cannot annotate consequences for invalid variants: " + variant.toString());

    //init
	bool plus_strand = transcript.isPlusStrand();
	VariantConsequence hgvs;

	//handle NMD transcripts
	if (transcript.biotype() == Transcript::BIOTYPE::NONSENSE_MEDIATED_DECAY)
	{
		hgvs.types.insert(VariantConsequenceType::NMD_TRANSCRIPT_VARIANT);
	}

	//normalization and 3' shifting for indel variants (MNPs are converted into InDels in this step)
	variant.normalize(plus_strand ? VcfLine::ShiftDirection::RIGHT : VcfLine::ShiftDirection::LEFT, genome_idx_, true, true);
	int start = variant.start();
	int end = variant.end();
	hgvs.normalized = variant.toString();
	if (debug) qDebug() << "variant normalized: " << variant.toString();

	//annotated exon and intron number (first affected exon/intron is reported)
	annotateExonIntronNumber(hgvs, transcript, variant, debug);

    Sequence ref = variant.ref();
    Sequence obs = variant.alt().at(0);
	QByteArray pos_hgvs_c;
	QByteArray pos_hgvs_c_dup;

    // annotate coding transcript
    if(transcript.isCoding())
    {
        if(variant.isSNV())
		{
			pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start, false, debug);
        }
        //deletion
        else if(variant.isDel())
		{
			pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start + 1, false, debug);

            if(end - start > 1 && pos_hgvs_c != "")
            {
                if(plus_strand)
                {
					pos_hgvs_c += "_" + annotateRegionsCoding(transcript, hgvs, end, false, debug);
                }
                else
                {
					pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, end, false, debug) + "_" + pos_hgvs_c;
				}
            }
        }
        //insertion
		else if(variant.isIns())
		{
			if(plus_strand)
			{
				//tandem duplication
				if(genome_idx_.seq(variant.chr(), start - variant.alt(0).length() + 2, variant.alt(0).length() - 1) == variant.alt(0).mid(1))
				{
					pos_hgvs_c_dup = annotateRegionsCoding(transcript, hgvs, start - variant.alt(0).length() + 2, true, debug);
					if(variant.alt(0).length() > 2)
					{
						pos_hgvs_c_dup += "_" + annotateRegionsCoding(transcript, hgvs, start, true, debug);
                    }

					//if the duplication does not overlap completely with the transcript we cannot treat the variant as duplication (check for empty cDNA positions)
					if(pos_hgvs_c_dup.startsWith('_') || pos_hgvs_c_dup.endsWith('_'))
					{
						pos_hgvs_c_dup = "";
					}
                }
				pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start, false) + "_" + annotateRegionsCoding(transcript, hgvs, start + 1, false, debug);

            }
			else
			{
				//tandem duplication
				if(genome_idx_.seq(variant.chr(), start + 1, variant.alt(0).length() - 1) == variant.alt(0).mid(1))
				{
					pos_hgvs_c_dup = annotateRegionsCoding(transcript, hgvs, start + variant.alt(0).length() - 1, true, debug);
					if(variant.alt(0).length() > 2)
					{
						pos_hgvs_c_dup += "_" + annotateRegionsCoding(transcript, hgvs, start + 1, true, debug);
					}

					//if the duplication does not overlap completely with the transcript we cannot treat the variant as duplication (check for empty cDNA positions)
					if(pos_hgvs_c_dup.startsWith('_') || pos_hgvs_c_dup.endsWith('_'))
					{
						pos_hgvs_c_dup = "";
					}
                }
				pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start + 1, false, debug) + "_" +  annotateRegionsCoding(transcript, hgvs, start, false, debug);
            }
            if(pos_hgvs_c == "_") pos_hgvs_c = "";
            if(pos_hgvs_c_dup == "_") pos_hgvs_c_dup = "";
        }
		//deletions and insertions
		else if(variant.isInDel())
		{
			pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, start + 1, false, debug);

            if(end - start > 1 && pos_hgvs_c != "")
            {
                if(plus_strand)
                {
					pos_hgvs_c += "_" + annotateRegionsCoding(transcript, hgvs, end, false, debug);
                }
                else
                {
					pos_hgvs_c = annotateRegionsCoding(transcript, hgvs, end, false, debug) + "_" + pos_hgvs_c;
                }
			}
        }
		else
		{
			THROW(ArgumentException, "Could not determine type of coding variant " + variant.toString());
		}
		if (debug) qDebug() << "annotate" << __LINE__ << "hgvs_c:" << hgvs.hgvs_c << "hgvs_p:" << hgvs.hgvs_p << "types:" << hgvs.typesToString();

        // create HGVS protein annotation
		if(pos_hgvs_c!="" && hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT))
        {
			if(!variant.isIns() && hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT))
            {
				//special case: deletion/indel spanning exon-intron boundary -> no protein annotation
			}
			else
			{
				hgvs.hgvs_p = getHgvsProteinAnnotation(variant, pos_hgvs_c, transcript, debug);
				if (debug) qDebug() << "annotate" << __LINE__ << "hgvs_c:" << hgvs.hgvs_c << "hgvs_p:" << hgvs.hgvs_p << "types:" << hgvs.typesToString();
            }
        }
    }
    // non-coding transcript
    else
    {
        if(variant.isSNV())
		{
			pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start);
        }
        else if(variant.isDel())
		{
			pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start + 1);

            if(end - start > 1 && pos_hgvs_c != "")
            {
                if(plus_strand)
                {
					pos_hgvs_c += "_" + annotateRegionsNonCoding(transcript, hgvs, end);
                }
                else
                {
					pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, end) + "_" + pos_hgvs_c;
                }
            }
        }
        else if(variant.isIns())
		{
			if(plus_strand)
			{
				//tandem duplication
				if(genome_idx_.seq(variant.chr(), start - variant.alt(0).length() + 2, variant.alt(0).length() - 1) == variant.alt(0).mid(1))
				{
					pos_hgvs_c_dup = annotateRegionsNonCoding(transcript, hgvs, start - variant.alt(0).length() + 2);
					if(variant.alt(0).length() > 2)
                    {
						pos_hgvs_c_dup += "_" + annotateRegionsNonCoding(transcript, hgvs, start);
                    }

					//if the duplication does not overlap completely with the transcript we cannot treat the variant as duplication (check for empty cDNA positions)
					if(pos_hgvs_c_dup.startsWith('_') || pos_hgvs_c_dup.endsWith('_'))
					{
						pos_hgvs_c_dup = "";
					}
				}
				pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start) + "_" + annotateRegionsNonCoding(transcript, hgvs, start + 1);
            }
            else
			{
				//tandem duplication
				if(genome_idx_.seq(variant.chr(), start + 1, variant.alt(0).length() - 1) == variant.alt(0).mid(1))
                {
					pos_hgvs_c_dup = annotateRegionsNonCoding(transcript, hgvs, start + variant.alt(0).length() - 1, true);
					if(variant.alt(0).length() > 2)
                    {
						pos_hgvs_c_dup += "_" + annotateRegionsNonCoding(transcript, hgvs, start + 1, true);
                    }

					//if the duplication does not overlap completely with the transcript we cannot treat the variant as duplication (check for empty cDNA positions)
					if(pos_hgvs_c_dup.startsWith('_') || pos_hgvs_c_dup.endsWith('_'))
					{
						pos_hgvs_c_dup = "";
					}
                }
				pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start + 1) + "_" + annotateRegionsNonCoding(transcript, hgvs, start);
            }
            if(pos_hgvs_c == "_") pos_hgvs_c = "";
            if(pos_hgvs_c_dup == "_") pos_hgvs_c_dup = "";
        }
		else if(variant.isInDel())
        {
			pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, start + 1);

            if(end - start > 1 && pos_hgvs_c != "")
            {
                if(plus_strand)
                {
					pos_hgvs_c += "_" + annotateRegionsNonCoding(transcript, hgvs, end);
                }
                else
                {
					pos_hgvs_c = annotateRegionsNonCoding(transcript, hgvs, end) + "_" + pos_hgvs_c;
                }
            }
        }
		else
		{
			THROW(ArgumentException, "Could not determine type of non-coding variant " + variant.toString());
		}
	}
	if (debug) qDebug() << "annotate" << __LINE__ << "hgvs_c:" << hgvs.hgvs_c << "hgvs_p:" << hgvs.hgvs_p << "types:" << hgvs.typesToString();

	//up- or downstream variant, no description of cDNA positions possible
	bool inside_transcript = start>=transcript.start() && end<=transcript.end(); //check if variant is completely inside transcript. If not, no cDNA change should be returned.
	if(pos_hgvs_c=="" || !inside_transcript)
	{
		hgvs.hgvs_c = "";
		hgvs.hgvs_p = "";
		hgvs.impact = "MODIFIER";
		return hgvs;
	}

	//find out if the variant is a splice region variant
	int start_affected = start;
	int end_affected = end;
	if(variant.isDel() || variant.isInDel()) //deletion/deletion-insertion: change is after prefix base > adjust start (end is correct)
	{
		++start_affected;
	}
	annotateSpliceRegion(hgvs, transcript, start_affected, end_affected, variant.isIns(), debug);
	if (debug) qDebug() << "annotate" << __LINE__ << "hgvs_c:" << hgvs.hgvs_c << "hgvs_p:" << hgvs.hgvs_p << "types:" << hgvs.typesToString();

	QByteArray hgvs_c_prefix = transcript.isCoding() ? "c." : "n.";

    //SNV
    if(variant.isSNV())
    {
		if(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT))
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
        Sequence alt = variant.alt(0).mid(1);
        //duplication
        if(pos_hgvs_c_dup != "")
        {
             hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c_dup + "dup";
        }
        //insertion
        else
        {
            if(!plus_strand) alt.reverseComplement();
            hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + "ins" + alt;
        }
    }
    //delins (deletion and insertion at the same time/substitution of more than one base)
	else if(variant.isInDel())
    {
        Sequence alt = variant.alt(0).mid(1);
        if(!plus_strand) alt.reverseComplement();
        hgvs.hgvs_c = hgvs_c_prefix + pos_hgvs_c + "delins" + alt;
    }
	if (debug) qDebug() << "annotate" << __LINE__ << "hgvs_c:" << hgvs.hgvs_c << "hgvs_p:" << hgvs.hgvs_p << "types:" << hgvs.typesToString();

    //consequence annotations based on protein annotation string
    if(!variant.isSNV() && hgvs.hgvs_p != "")
    {
		hgvs.types.insert(VariantConsequenceType::PROTEIN_ALTERING_VARIANT);

        //effects on stop or start codon
		if(hgvs.types.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT) && hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT))
        {
			hgvs.types.insert(VariantConsequenceType::STOP_LOST);
        }

        if(hgvs.hgvs_p == "p.Met1?")
        {
			hgvs.types.insert(VariantConsequenceType::START_LOST);
        }
        else if(hgvs.hgvs_p == "p.Met1=")
        {
			hgvs.types.insert(VariantConsequenceType::START_RETAINED_VARIANT);
        }
        else if(hgvs.hgvs_p.endsWith("Ter"))
        {
			hgvs.types.insert(VariantConsequenceType::STOP_GAINED);
        }
        else if(hgvs.hgvs_p.contains("Ter"))
        {
			hgvs.types.insert(VariantConsequenceType::STOP_LOST);
        }

        if(hgvs.hgvs_p.contains("fs"))
        {
			hgvs.types.insert(VariantConsequenceType::FRAMESHIFT_VARIANT);
        }
		else if(variant.isDel() && !hgvs.types.contains(VariantConsequenceType::START_RETAINED_VARIANT))
        {
			hgvs.types.insert(VariantConsequenceType::INFRAME_DELETION);
        }
        else if(variant.isIns())
        {
			hgvs.types.insert(VariantConsequenceType::INFRAME_INSERTION);
        }
        else
        {
			if(!hgvs.types.contains(VariantConsequenceType::START_RETAINED_VARIANT))
            {
                if(variant.ref().length() == variant.alt(0).length())
                {
					hgvs.types.insert(VariantConsequenceType::MISSENSE_VARIANT);
                }
                else if(variant.ref().length() > variant.alt(0).length())
                {
					hgvs.types.insert(VariantConsequenceType::INFRAME_DELETION);
                }
                else
                {
					hgvs.types.insert(VariantConsequenceType::INFRAME_INSERTION);
                }
            }
        }
    }
	if (debug) qDebug() << "annotate" << __LINE__ << "hgvs_c:" << hgvs.hgvs_c << "hgvs_p:" << hgvs.hgvs_p << "types:" << hgvs.typesToString();

	//determine max impact
	QSet<QByteArray> impact_set;
	foreach(VariantConsequenceType type, hgvs.types)
	{
		impact_set << consequenceTypeToImpact(type);
	}
	if (impact_set.contains("HIGH")) hgvs.impact = "HIGH";
	else if (impact_set.contains("MODERATE")) hgvs.impact = "MODERATE";
	else if (impact_set.contains("LOW")) hgvs.impact = "LOW";
	else hgvs.impact = "MODIFIER";

    return hgvs;
}

//convert a variant in GSvar format into an HgvsNomenclature object
VariantConsequence VariantHgvsAnnotator::annotate(const Transcript& transcript, const Variant &variant, bool debug)
{
	VcfLine vcf_variant = variant.toVCF(genome_idx_);

	return annotate(transcript, vcf_variant, debug);
}

QByteArray VariantHgvsAnnotator::consequenceTypeToImpact(VariantConsequenceType type)
{
	switch(type)
	{
		case VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT:
		case VariantConsequenceType::SPLICE_DONOR_VARIANT:
		case VariantConsequenceType::STOP_GAINED:
		case VariantConsequenceType::FRAMESHIFT_VARIANT:
		case VariantConsequenceType::STOP_LOST:
		case VariantConsequenceType::START_LOST:
			return "HIGH";
			break;
		case VariantConsequenceType::INFRAME_INSERTION:
		case VariantConsequenceType::INFRAME_DELETION:
		case VariantConsequenceType::MISSENSE_VARIANT:
		case VariantConsequenceType::PROTEIN_ALTERING_VARIANT:
			return "MODERATE";
			break;
		case VariantConsequenceType::SPLICE_REGION_VARIANT:
		case VariantConsequenceType::INCOMPLETE_TERMINAL_CODON_VARIANT:
		case VariantConsequenceType::START_RETAINED_VARIANT:
		case VariantConsequenceType::STOP_RETAINED_VARIANT:
		case VariantConsequenceType::SYNONYMOUS_VARIANT:
			return "LOW";
			break;
		case VariantConsequenceType::CODING_SEQUENCE_VARIANT:
		case VariantConsequenceType::FIVE_PRIME_UTR_VARIANT:
		case VariantConsequenceType::THREE_PRIME_UTR_VARIANT:
		case VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT:
		case VariantConsequenceType::INTRON_VARIANT:
		case VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT:
		case VariantConsequenceType::UPSTREAM_GENE_VARIANT:
		case VariantConsequenceType::DOWNSTREAM_GENE_VARIANT:
		case VariantConsequenceType::INTERGENIC_VARIANT:
		case VariantConsequenceType::NMD_TRANSCRIPT_VARIANT:
			return "MODIFIER";
			break;
	}
	THROW(ProgrammingException, "Unhandled variant consequence type " + QString::number(static_cast<int>(type)) + "!");
}

// make variant consequence type annotations depending on the part of the transcript the variant occurs in;
// for coding variants and a single genomic position
QByteArray VariantHgvsAnnotator::annotateRegionsCoding(const Transcript& transcript, VariantConsequence& hgvs, int gen_pos, bool is_dup, bool debug)
{
	if (debug) qDebug() << "  annotating regions coding - gen_pos:" << gen_pos << " is_dup:" << is_dup;
	QByteArray pos_hgvs_c;
	bool plus_strand = transcript.isPlusStrand();

    //upstream of start codon
	if((plus_strand && gen_pos < transcript.codingStart()) || (!plus_strand && gen_pos > transcript.codingStart()))
	{
		if (debug) qDebug() << "  upstream of start codon";
        //in 5 prime utr or upstream variant?
		if((plus_strand && gen_pos >= transcript.start()) || (!plus_strand && gen_pos <= transcript.end()))
        {
			pos_hgvs_c = getHgvsPosition(transcript.utr5prime(), gen_pos, plus_strand, transcript.codingRegions(), true);

            //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
            if(!is_dup)
            {
                if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
                {
					hgvs.types.insert(VariantConsequenceType::INTRON_VARIANT);
                }
                else
                {
					hgvs.types.insert(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT);
                }
            }

            if(pos_hgvs_c.startsWith("+1-"))
            {
                pos_hgvs_c = pos_hgvs_c.mid(1);
            }
            else if(!pos_hgvs_c.startsWith("-1+"))
            {
                pos_hgvs_c = "-" + pos_hgvs_c;
            }
        }
		else if((plus_strand && transcript.start() - gen_pos <= params_.max_dist_to_transcript) || (!plus_strand && gen_pos - transcript.end() <= params_.max_dist_to_transcript))
        {

            //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
            if(!is_dup)
            {
				hgvs.types.insert(VariantConsequenceType::INTERGENIC_VARIANT);
				hgvs.types.insert(VariantConsequenceType::UPSTREAM_GENE_VARIANT);
            }
            return "";
        }
        else
        {
			return "";
        }
    }
    //downstream of stop codon
	else if((plus_strand && gen_pos > transcript.codingEnd()) ||  (!plus_strand && gen_pos < transcript.codingEnd()))
	{
		if (debug) qDebug() << "  downstream of stop codon";
		//in 3 prime utr or downstream variant?
		if((plus_strand && gen_pos <= transcript.end()) || (!plus_strand && gen_pos >= transcript.start()))
        {
            //determine number of first exon in 3 prime utr
            int utr_5_count = transcript.utr5prime().count();
            int utr_3_count = transcript.utr3prime().count();
            int coding_count = transcript.codingRegions().count();
            int first_region = utr_5_count + coding_count;

            //account for exons shared between cds and 5/3 prime utr if applicable
            if(utr_5_count >= 1)
            {
                if((plus_strand && transcript.codingStart() - transcript.utr5prime()[utr_5_count - 1].end() == 1) ||
                        (!plus_strand && transcript.utr5prime()[0].start() - transcript.codingStart() == 1))
                {
                    first_region--;
                }
            }
            if(utr_3_count >= 1)
            {
                if((plus_strand && transcript.utr3prime()[0].start() - transcript.codingEnd() == 1) ||
                        (!plus_strand && transcript.codingEnd() - transcript.utr3prime()[utr_3_count - 1].end() == 1))
                {
                    first_region--;
                }
            }

			pos_hgvs_c = getHgvsPosition(transcript.utr3prime(), gen_pos, plus_strand, transcript.codingRegions(), false);
            if(pos_hgvs_c.startsWith("+"))
            {
                int cds_length = 0;
                for(int i = 0; i < transcript.codingRegions().count(); i++)
                {
                    cds_length += transcript.codingRegions()[i].length();
                }
				pos_hgvs_c = QByteArray::number(cds_length) + pos_hgvs_c;
            }
            else
            {
                pos_hgvs_c = "*" + pos_hgvs_c;
            }

            //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
            if(!is_dup)
            {
                if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
                {
					hgvs.types.insert(VariantConsequenceType::INTRON_VARIANT);
                }
                else
                {
					hgvs.types.insert(VariantConsequenceType::THREE_PRIME_UTR_VARIANT);
                }
            }
        }
		else if((plus_strand && gen_pos - transcript.end() <= params_.max_dist_to_transcript) || (!plus_strand && transcript.start() - gen_pos <= params_.max_dist_to_transcript))
        {
            //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
            if(!is_dup)
            {
				hgvs.types.insert(VariantConsequenceType::INTERGENIC_VARIANT);
				hgvs.types.insert(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT);
            }
            return "";
        }
        else
        {
			return "";
        }
    }
    //between start and stop codon
    else
    {
		if (debug) qDebug() << "  between start/end";
		//determine number of first exon in coding sequence; subtract 1 because of exon that is both utr and cds
        int utr_5_count = transcript.utr5prime().count();
        int first_region = std::max(utr_5_count - 1, 0);

        //account for special case where UTR and CDS are separated by an intron
        if(utr_5_count >= 1)
        {
            if((plus_strand && transcript.codingStart() - transcript.utr5prime()[utr_5_count - 1].end() > 1) ||
                    (!plus_strand && transcript.utr5prime()[0].start() - transcript.codingStart() > 1))
            {
                first_region++;
            }
        }

		pos_hgvs_c = getHgvsPosition(transcript.codingRegions(), gen_pos, plus_strand, transcript.codingRegions(), false);

        //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
        if(!is_dup)
        {
            if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
            {
				hgvs.types.insert(VariantConsequenceType::INTRON_VARIANT);
            }
            else
            {
				hgvs.types.insert(VariantConsequenceType::CODING_SEQUENCE_VARIANT);
            }
        }
    }
    return pos_hgvs_c;
}

// make variant consequence type annotations depending on the part of the transcript the variant occurs in;
// for non-coding variants and a single genomic position
QByteArray VariantHgvsAnnotator::annotateRegionsNonCoding(const Transcript& transcript, VariantConsequence& hgvs, int gen_pos, bool is_dup)
{
	bool plus_strand = transcript.isPlusStrand();

	QByteArray pos_hgvs_c;

    if(gen_pos >= transcript.start() && gen_pos <= transcript.end())
    {
        //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
        if(!is_dup)
        {
			hgvs.types.insert(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT);
        }

		pos_hgvs_c = getHgvsPosition(transcript.regions(), gen_pos, plus_strand, transcript.regions(), false);

        //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
        if(!is_dup)
        {
            if(pos_hgvs_c.contains("+") || pos_hgvs_c.contains("-"))
            {
				hgvs.types.insert(VariantConsequenceType::INTRON_VARIANT);
            }
            else
            {
				hgvs.types.insert(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT);
            }
        }
    }
    // variant downstream of non-coding transcript
	else if((plus_strand && gen_pos - transcript.end() <= params_.max_dist_to_transcript && gen_pos > transcript.end()) || (!plus_strand && transcript.start() - gen_pos <= params_.max_dist_to_transcript && gen_pos < transcript.start()))
    {
        //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
        if(!is_dup)
        {
			hgvs.types.insert(VariantConsequenceType::INTERGENIC_VARIANT);
			hgvs.types.insert(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT);
        }
        return "";
    }
    // variant upstream of non-coding transcript
	else if((plus_strand && transcript.start() - gen_pos <= params_.max_dist_to_transcript && gen_pos < transcript.start()) || (!plus_strand && gen_pos - transcript.end() <= params_.max_dist_to_transcript && gen_pos > transcript.end()))
    {
        //if positions of duplicated regions are annotated, don't insert consequences (apply only to insertion position!)
        if(!is_dup)
        {
			hgvs.types.insert(VariantConsequenceType::INTERGENIC_VARIANT);
			hgvs.types.insert(VariantConsequenceType::UPSTREAM_GENE_VARIANT);
        }
        return "";
    }
    else
    {
		return "";
    }
    return pos_hgvs_c;
}

//determine the HGVS position string for a single genomic position in any part of the transcript
QByteArray VariantHgvsAnnotator::getHgvsPosition(const BedFile& regions, int gen_pos, bool plus_strand, const BedFile& coding_regions, bool utr_5)
{
    bool in_exon = false;

    int pos = 0;

	QByteArray pos_hgvs_c;

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
                break;
            }
            else
            {
                pos = regions[i].end() - gen_pos + 1;
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
		pos_hgvs_c = QByteArray::number(pos);
    }
    else
    {
		pos_hgvs_c = getPositionInIntron(regions, gen_pos, plus_strand, coding_regions, utr_5);
    }
    return pos_hgvs_c;
}

//determine the HGVS position string for a single genomic position in an intron
QByteArray VariantHgvsAnnotator::getPositionInIntron(const BedFile& regions, int genomic_position, bool plus_strand, const BedFile& coding_regions, bool utr_5)
{
	QByteArray pos_in_intron;
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
		if(genomic_position > regions[i].end() && genomic_position < regions[i+1].start())
        {
            pos_found = true;
            int prev_exon_end = regions[i].end();
            int next_exon_start = regions[i+1].start();

            int dist_below = genomic_position - prev_exon_end;
            int dist_above = next_exon_start - genomic_position;

            if(plus_strand)
            {
                if(utr_5)
                {
                    if(dist_below < dist_above)
                    {
						pos_in_intron = "-" + QByteArray::number(dist_below);
                    }
                    else
                    {
						pos_in_intron = "+" + QByteArray::number(dist_above);
                    }
                }
                else
                {
                    if(dist_below <= dist_above)
                    {
						pos_in_intron = "+" + QByteArray::number(dist_below);
                    }
                    else
                    {
						pos_in_intron = "-" + QByteArray::number(dist_above);
                    }
                }
                break;
            }
            else
            {
                closest_exon_pos += regions[i+1].length();

                if(utr_5)
                {
                    if(dist_above < dist_below)
                    {
						pos_in_intron = "-" + QByteArray::number(dist_above);
                    }
                    else
                    {
						pos_in_intron = "+" + QByteArray::number(dist_below);
                    }
                }
                else
                {
                    if(dist_above <= dist_below)
                    {
						pos_in_intron = "+" + QByteArray::number(dist_above);
                    }
                    else
                    {
						pos_in_intron = "-" + QByteArray::number(dist_below);
                    }
                }
            }
        }
    }

    //special case: utr and coding regions on completely different exons, position on intron between them
    if(!pos_found) //only happens in the case stated above, when regions are 5 or 3 prime utr
    {
        //position on intron between 5 prime utr and first cds exon
        if(utr_5)
        {
            //strand information was reverted for 5 prime utr
            if(plus_strand)
            {
                int dist_below = genomic_position - coding_regions[coding_regions.count()-1].end();
                int dist_above = regions[0].start() - genomic_position;

                if(dist_below < dist_above)
                {
					pos_in_intron = "+1-" + QByteArray::number(dist_below);
                }
                else
                {
					pos_in_intron = "-1+" + QByteArray::number(dist_above);
                }
            }
            else
            {
                int dist_below = genomic_position - regions[regions.count()-1].end();
                int dist_above = coding_regions[0].start() - genomic_position;

                if(dist_below <= dist_above)
                {
					pos_in_intron = "-1+" + QByteArray::number(dist_below);
                }
                else
                {
					pos_in_intron = "+1-" + QByteArray::number(dist_above);
                }
            }
        }
        //position on intron between last cds exon and 3 prime utr
        else
        {
            if(plus_strand)
            {
                int dist_below = genomic_position - coding_regions[coding_regions.count()-1].end();
                int dist_above = regions[0].start() - genomic_position;

                if(dist_below <= dist_above)
                {
					pos_in_intron = "+" + QByteArray::number(dist_below);
                }
                else
                {
					pos_in_intron = "1-" + QByteArray::number(dist_above);
                }
            }
            else
            {
                int dist_below = genomic_position - regions[regions.count()-1].end();
                int dist_above = coding_regions[0].start() - genomic_position;

                if(dist_below < dist_above)
                {
					pos_in_intron = "1-" + QByteArray::number(dist_below);
                }
                else
                {
					pos_in_intron = "+" + QByteArray::number(dist_above);
                }
            }
        }
        return pos_in_intron;
    }

    //annotate position according to closest exon
    if(pos_in_intron.startsWith("+"))
    {
        if(utr_5)
        {
			pos_in_intron = QByteArray::number(closest_exon_pos+1) + pos_in_intron;
        }
        else
        {
			pos_in_intron = QByteArray::number(closest_exon_pos) + pos_in_intron;
        }
    }
    else if(pos_in_intron.startsWith("-"))
    {
        if(utr_5)
        {
			pos_in_intron = QByteArray::number(closest_exon_pos) + pos_in_intron;
        }
        else
        {
			pos_in_intron = QByteArray::number(closest_exon_pos+1) + pos_in_intron;
        }
    }

    return pos_in_intron;
}

//translate a DNA sequence into an amino acid sequence
QByteArray VariantHgvsAnnotator::translate(const Sequence& seq, bool is_mito, bool end_at_stop)
{
    if(seq.length() % 3 != 0) THROW(ArgumentException, "Coding sequence length must be multiple of three.")

    QByteArray aa_seq;

    for(int i=0; i<seq.length(); i+=3)
    {
		aa_seq.append(NGSHelper::translateCodonThreeLetterCode(seq.mid(i, 3), is_mito));

        //only translate up to termination codon
        if(end_at_stop && aa_seq.right(3) == "Ter") break;
    }
    return aa_seq;
}

//determine the annotation of the variant according to the HGVS nomenclature for proteins
QByteArray VariantHgvsAnnotator::getHgvsProteinAnnotation(const VcfLine& variant, const QByteArray& pos_hgvs_c, const Transcript& transcript, bool debug)
{
	if (debug) qDebug() << "getHgvsProteinAnnotation - determining protein change for " << pos_hgvs_c;

	bool plus_strand = transcript.isPlusStrand();
	bool use_mito_table = variant.chr().isM();

	QByteArray hgvs_p("p.");
    int pos_trans_start = 0;
    int start = variant.start();
    int end = variant.end();
    QByteArray aa_ref;
    QByteArray aa_obs;
    Sequence seq_obs;
	Sequence coding_sequence = getCodingSequence(transcript, true);

    if(variant.isSNV())
	{
        //make position in transcript sequence zero-based
        pos_trans_start = pos_hgvs_c.toInt() - 1;
        int offset = pos_trans_start % 3;

        //translate the reference sequence codon and obtain the observed sequence codon
		aa_ref = NGSHelper::translateCodonThreeLetterCode(coding_sequence.mid(pos_trans_start - offset, 3), use_mito_table);
        seq_obs = coding_sequence.mid(pos_trans_start - offset, 3);
        if(plus_strand)
        {
            seq_obs[offset] = variant.alt().at(0)[0];
        }
        else
        {
            seq_obs[offset] = variant.alt().at(0).toReverseComplement()[0];
        }

        //translate the observed sequence codon
		aa_obs = NGSHelper::translateCodonThreeLetterCode(seq_obs, use_mito_table);

        if(aa_obs == aa_ref)
        {
            aa_obs = "=";
        }
        else if(aa_ref == "Met" && pos_trans_start < 3)
        {
            aa_obs = "?";
        }
        else if(aa_ref == "Ter")
        {
            aa_obs.append("extTer");
            bool stop_found = false;
            for(int i = pos_trans_start - offset + 3; i < coding_sequence.length() - 2; i += 3)
            {
				if(NGSHelper::translateCodon(coding_sequence.mid(i, 3), use_mito_table) == '*')
                {
                    stop_found = true;
                    int stop_pos = i - (pos_trans_start - offset);
                    aa_obs.append(QByteArray::number(stop_pos / 3));
                    break;
                }
            }
            if(!stop_found)
            {
                aa_obs.append("?");
            }
        }

        aa_ref.append(QByteArray::number(pos_trans_start / 3 + 1));
    }
	else if(variant.isIns() || variant.isDel() || variant.isInDel())
    {
		if(variant.isIns() && pos_hgvs_c == "-1_1") return "";

		QByteArrayList positions = pos_hgvs_c.split('_');

        //make position of start of deletion/insertion in transcript zero-based
        pos_trans_start = positions.at(0).toInt() - 1;

        //don't annotate deletions spanning two (or more) exons
        if(positions.size() == 2 && variant.isDel())
		{
			if(transcript.exonNumber(variant.start()+1, variant.end())==-2)
            {
				if (debug) qDebug() << " Spans two exons => no protein change annotated!";
                return "p.?";
            }
		}

        //special case: deletion spanning 5' UTR and start of coding region
        if(pos_trans_start <= -1)
        {
            int pos_trans_end = positions.at(1).toInt() - 1;
            if(pos_trans_end > 2)
            {
                return "p.Met1?";
            }
            else
            {
                Sequence new_start;
                if(plus_strand)
                {
                    if(variant.isDel())
                    {
						new_start = genome_idx_.seq(variant.chr(), variant.start() - pos_trans_end, pos_trans_end + 1);
                    }
                    else
                    {
                        new_start = variant.alt(0).right(pos_trans_end + 1);
                    }
                }
                else
                {
                    if(variant.isDel())
                    {
						new_start = genome_idx_.seq(variant.chr(), variant.end() + pos_trans_end + 1, pos_trans_end + 1);
                    }
                    else
                    {
                        new_start = variant.alt(0).mid(1, pos_trans_end + 1);
                    }
                    new_start.reverseComplement();
                }
                if(new_start == coding_sequence.left(pos_trans_end + 1))
                {
                    return "p.Met1=";
                }
                else
                {
                    return "p.Met1?";
                }
            }
		}

        int offset = pos_trans_start % 3;
        int frame_diff = variant.isDel() ? end - start : variant.alt()[0].length() - variant.ref().length();
		int pos_shift = 0;

		if (debug) qDebug() << "getHgvsProteinAnnotation" << __LINE__<< "offset:" << offset << "frame_diff:" << frame_diff;

        // get reference and observed sequence (from coding sequence)
        Sequence seq_ref = coding_sequence.mid(pos_trans_start - offset);
		if (variant.isSNV())
		{
			Sequence alt = variant.alt(0);
			if(!plus_strand) alt.reverseComplement();
			seq_obs = seq_ref.left(offset) + alt + seq_ref.mid(offset + variant.ref().length());
		}
		else if(variant.isDel())
		{
			seq_obs = seq_ref.left(offset) + seq_ref.mid(offset + frame_diff);
        }
        else if(variant.isIns())
		{
			Sequence alt = variant.alt(0).mid(1);
            if(!plus_strand) alt.reverseComplement();
            seq_obs = seq_ref.left(offset + 1) + alt + seq_ref.mid(offset + 1);
        }
		else //indel
		{
			Sequence alt = variant.alt(0).mid(1);
            if(!plus_strand) alt.reverseComplement();
			seq_obs = seq_ref.left(offset) + alt + seq_ref.mid(offset + variant.ref().length() - 1);
		}

		if (debug) qDebug() << "  ref: " << seq_ref;
		if (debug) qDebug() << "  obs: " << seq_obs;

		if(variant.isDel() || (variant.isIns() && frame_diff % 3 != 0) || variant.isInDel())
		{
			//find the first amino acid that is changed due to the deletion/frameshift insertion/deletion-insertion
            while(aa_obs == aa_ref && aa_obs != "Ter" && aa_ref != "Ter")
            {
				aa_ref = NGSHelper::translateCodonThreeLetterCode(seq_ref.left(3), use_mito_table);
				aa_obs = NGSHelper::translateCodonThreeLetterCode(seq_obs.left(3), use_mito_table);

                if(aa_obs == aa_ref && aa_obs != "Ter")
                {
                    seq_obs = seq_obs.mid(3);
                    seq_ref = seq_ref.mid(3);
                    pos_shift += 3;
					if (debug) qDebug() << __LINE__ << pos_shift;
                }
            }
            aa_ref.append(QByteArray::number((pos_trans_start + pos_shift) / 3 + 1));
        }
        else if(variant.isIns())
        {
			QByteArray aa_ref_next;
            QByteArray aa_obs_next;

            //shift to the most C-terminal position possible
            while(aa_obs == aa_ref && aa_obs_next == aa_ref_next && aa_obs != "Ter" && aa_ref != "Ter")
            {
				aa_ref = NGSHelper::translateCodonThreeLetterCode(seq_ref.left(3), use_mito_table);
				aa_obs = NGSHelper::translateCodonThreeLetterCode(seq_obs.left(3), use_mito_table);
				aa_ref_next = NGSHelper::translateCodonThreeLetterCode(seq_ref.mid(3, 3), use_mito_table);
				aa_obs_next = NGSHelper::translateCodonThreeLetterCode(seq_obs.mid(3, 3), use_mito_table);

                if(aa_obs == aa_ref && aa_obs_next == aa_ref_next && aa_obs != "Ter")
                {
                    seq_obs = seq_obs.mid(3);
                    seq_ref = seq_ref.mid(3);
                    pos_shift += 3;
                }
            }

            //check for duplication
            int diff = 0;
            if(aa_obs == aa_ref) diff += 3;

			QByteArray aa_ref_after = NGSHelper::translateCodonThreeLetterCode(seq_ref.mid(diff, 3), use_mito_table);
			QByteArray aa_obs_after = NGSHelper::translateCodonThreeLetterCode(seq_obs.mid(diff + frame_diff, 3), use_mito_table);

			QByteArray inserted_sequence = translate(seq_obs.mid(diff, frame_diff));
			QByteArray left_sequence;
            if(pos_trans_start + pos_shift - offset - frame_diff > 0)
            {
                left_sequence = translate(coding_sequence.mid(pos_trans_start + pos_shift - offset - frame_diff + diff, frame_diff));
            }
            if(inserted_sequence == left_sequence)
            {
				aa_ref = left_sequence.left(3);
                aa_ref.append(QByteArray::number((pos_trans_start + pos_shift - offset - frame_diff + diff) / 3 + 1));
                if(left_sequence.length() > 3)
                {
                    aa_ref.append("_" + left_sequence.right(3));
                    aa_ref.append(QByteArray::number((pos_trans_start + pos_shift - offset + diff) / 3));
                }
                aa_obs = "dup";
            }
            else if(aa_obs == aa_ref && aa_obs_after == aa_ref_after)
            {
                aa_ref.append(QByteArray::number((pos_trans_start + pos_shift) / 3 + 1));
				aa_ref += "_" + NGSHelper::translateCodonThreeLetterCode(seq_ref.mid(3, 3)) + QByteArray::number((pos_trans_start + pos_shift) / 3 + 2);
				aa_obs = "ins" + inserted_sequence;
            }
            else if(aa_obs_after == aa_ref && pos_trans_start + pos_shift - offset > 2)
            {
                aa_ref = translate(coding_sequence.mid(pos_trans_start + pos_shift - offset - 3, 3)) +
                        QByteArray::number((pos_trans_start + pos_shift) / 3) +
                        "_" + aa_ref + QByteArray::number((pos_trans_start + pos_shift) / 3 + 1);
				aa_obs = "ins" + inserted_sequence;
            }
            //delins if amino acid is changed by the insertion
            else
            {
                //extension if changed amino acid was stop codon
                if(aa_ref == "Ter")
                {
                    aa_obs.append("extTer");
                    bool stop_found = false;
                    for(int i = 3; i < seq_obs.length() - 2; i += 3)
                    {
						if(NGSHelper::translateCodon(seq_obs.mid(i, 3), use_mito_table) == '*')
                        {
                            stop_found = true;
                            aa_obs.append(QByteArray::number(i / 3));
                            break;
                        }
                    }
                    if(!stop_found)
                    {
                        aa_obs.append("?");
                    }
                }
                else
                {
                    aa_obs = "delins" + translate(seq_obs.left(3 + frame_diff));
                }
                aa_ref.append(QByteArray::number((pos_trans_start + pos_shift) / 3 + 1));
            }
        }
        else
        {
			aa_ref.append(QByteArray::number((pos_trans_start + pos_shift) / 3 + 1));
		}

        // frameshift deletion/insertion/deletion-insertion
        if(frame_diff % 3 != 0)
        {
			if (debug) qDebug() << __LINE__;
			//special case if deleted amino acid is Met at translation start site
            if(aa_ref == "Met1")
            {
                aa_obs = "?";
            }
            //special case if first changed amino acid is changed into a stop codon: describe as substitution
            else if(aa_obs != "Ter")
            {
               aa_obs = aa_obs + "fs" + "Ter";

               //search for stop codon in new reading frame
               bool stop_found = false;
               for(int i = 3; i < seq_obs.length() - 2; i += 3)
               {
				   if(NGSHelper::translateCodon(seq_obs.mid(i, 3), use_mito_table) == '*')
                   {
                       stop_found = true;
                       aa_obs.append(QByteArray::number(i / 3 + 1));
                       break;
                   }
			   }
               if(!stop_found)
               {
                   aa_obs.append("?");
               }
            }
        }
        // inframe deletion
        else if(variant.isDel())
        {
			if (debug) qDebug() << __LINE__;
			// more than one amino acid deleted or delins with 3 deleted bases
			if(frame_diff > 3 || aa_obs != NGSHelper::translateCodonThreeLetterCode(seq_ref.mid(frame_diff, 3)))
            {
                int deletion_length = frame_diff;
                aa_ref.append("_");
				if(aa_obs == NGSHelper::translateCodonThreeLetterCode(seq_ref.mid(frame_diff, 3)))
                {
                    pos_shift -= 3;
                }
                else
                {
                    deletion_length += 3;
                }

				QByteArray deleted_aa_seq = translate(seq_ref.left(deletion_length), use_mito_table, true);

                if(deleted_aa_seq.endsWith("Ter"))
                {
                    aa_ref.append("Ter" + QByteArray::number((pos_trans_start + pos_shift + deleted_aa_seq.length()) / 3 + 1));
                }
                else
                {
					aa_ref.append(NGSHelper::translateCodonThreeLetterCode(coding_sequence.mid(pos_trans_start - offset + pos_shift + frame_diff, 3), use_mito_table));
                    aa_ref.append(QByteArray::number((pos_trans_start + pos_shift + frame_diff) / 3 + 1));
                }
            }

            if(aa_ref.startsWith("Ter"))
            {
                aa_obs.append("extTer");

                //search for stop codon in extension
                bool stop_found = false;
                for(int i = 3; i < seq_obs.length() - 2; i += 3)
                {
					if(NGSHelper::translateCodon(seq_obs.mid(i, 3), use_mito_table) == '*')
                    {
                        stop_found = true;
                        aa_obs.append(QByteArray::number(i / 3 + 1));
                        break;
                    }
                }
                if(!stop_found)
                {
                    aa_obs.append("?");
                }
            }
            // delins if first mismatched amino acid is not the first one after the deletion
			else if(aa_obs != NGSHelper::translateCodonThreeLetterCode(seq_ref.mid(frame_diff, 3)))
            {
                aa_obs = "delins" + aa_obs;
            }
            else
            {
                aa_obs = "del";
            }
        }
        //inframe deletion-insertion, more than one amino acid deleted
        else if(!variant.isIns() && variant.ref().length() > (4 + pos_shift))
        {
			if (debug) qDebug() << __LINE__;
			int offset_end = (offset + variant.ref().length() - 1) % 3;
            aa_ref.append("_");
            if(plus_strand)
            {
				aa_ref.append(NGSHelper::translateCodonThreeLetterCode(genome_idx_.seq(variant.chr(), end - offset_end, 3), use_mito_table));
            }
            else
            {
				aa_ref.append(NGSHelper::translateCodonThreeLetterCode(genome_idx_.seq(variant.chr(), start - 2 + offset_end, 3).toReverseComplement(), use_mito_table));
            }
            aa_ref.append(QByteArray::number((pos_trans_start + variant.ref().length() - pos_shift - 1) / 3 + 1));

            //more than one amino acid inserted
            if(variant.alt(0).length() > (4 + pos_shift))
            {
                aa_obs = "delins" + translate(seq_obs.left((variant.alt(0).length() - 1) + 1  - pos_shift));
            }
            else
            {
                aa_obs = "delins" + aa_obs;
            }
        }
        //inframe deletion-insertion, more than one amino acid inserted
        else if(!variant.isIns() && variant.alt(0).length() > (4 + pos_shift))
        {
			if (debug) qDebug() << __LINE__ << aa_ref << aa_obs;
			if (debug) qDebug() << __LINE__ << pos_shift << ((variant.alt(0).length() - 1) + 1  - pos_shift);
			aa_obs = "delins" + translate(seq_obs.left((variant.alt(0).length() - 1) + 1  - pos_shift));
        }
    }

    hgvs_p.append(aa_ref);
    hgvs_p.append(aa_obs);

    return hgvs_p;
}


//add the consequence type according to the change in the protein sequence
void VariantHgvsAnnotator::annotateProtSeqCsqSnv(VariantConsequence& hgvs)
{
    if(hgvs.hgvs_p.endsWith("="))
    {
		hgvs.types.insert(VariantConsequenceType::SYNONYMOUS_VARIANT);
        if(hgvs.hgvs_p.contains("Ter"))
        {
			hgvs.types.insert(VariantConsequenceType::STOP_RETAINED_VARIANT);
        }
        else if(hgvs.hgvs_p == "p.Met1=")
        {
			hgvs.types.insert(VariantConsequenceType::START_RETAINED_VARIANT);
        }
        return;
    }
    else
    {
		hgvs.types.insert(VariantConsequenceType::PROTEIN_ALTERING_VARIANT);
    }

    if(hgvs.hgvs_p == "p.Met1?")
    {
		hgvs.types.insert(VariantConsequenceType::START_LOST);
    }
    else if(hgvs.hgvs_p.endsWith("Ter"))
    {
		hgvs.types.insert(VariantConsequenceType::STOP_GAINED);
    }
    else if(hgvs.hgvs_p.contains("Ter"))
    {
		hgvs.types.insert(VariantConsequenceType::STOP_LOST);
    }
    else
    {
		hgvs.types.insert(VariantConsequenceType::MISSENSE_VARIANT);
    }
}

//annotate if the variant is a splice region variant
void VariantHgvsAnnotator::annotateSpliceRegion(VariantConsequence& hgvs, const Transcript& transcript, int start, int end, bool insertion, bool debug)
{
	if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "start:" << start << "end:" << end;

	bool plus_strand = transcript.isPlusStrand();

    for(int i=0; i<transcript.regions().count(); i++)
    {
		const BedLine& reg = transcript.regions()[i];

		//5' splice region (beginning of exon)
		if ((plus_strand && i!=0) || (!plus_strand && i!=transcript.regions().count()-1)) //first exon does not have splice region at the beginning
		{
			int five_prime_reg_start = plus_strand ? reg.start() - params_.splice_region_in_5 : reg.end() - params_.splice_region_ex + 1;
			int five_prime_reg_end = plus_strand ? reg.start() + params_.splice_region_ex - 1: reg.end() + params_.splice_region_in_5;
			int acceptor_reg_start = plus_strand ? reg.start()-2 : reg.end()+1;
			int acceptor_reg_end = plus_strand ? reg.start()-1 : reg.end()+2;

			if (!insertion)
			{
				if (BasicStatistics::rangeOverlaps(start, end, five_prime_reg_start, five_prime_reg_end))
				{
					if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in 5' splice region :" << five_prime_reg_start << "-" << five_prime_reg_end;
					hgvs.types.insert(VariantConsequenceType::SPLICE_REGION_VARIANT);

					if (BasicStatistics::rangeOverlaps(start, end, acceptor_reg_start, acceptor_reg_end))
					{
						if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in splice acceptor:" << acceptor_reg_start << acceptor_reg_end;
						hgvs.types.insert(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT);
					}
				}
			}
			else //special handling for insertions (they affect no base directly)
			{
				if (BasicStatistics::rangeOverlaps(start, start+1, five_prime_reg_start+1, five_prime_reg_end-1))
				{
					if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in 5' splice region :" << five_prime_reg_start << "-" << five_prime_reg_end;
					hgvs.types.insert(VariantConsequenceType::SPLICE_REGION_VARIANT);

					if (start==acceptor_reg_start)
					{
						if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in splice acceptor:" << acceptor_reg_start << acceptor_reg_end;
						hgvs.types.insert(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT);
					}
				}
			}
		}

		//3' splice region (end of exon)
		if ((plus_strand && i!=transcript.regions().count()-1) || (!plus_strand && i!=0)) //last exon does not have splice region at the end
		{
			int three_prime_reg_start = plus_strand ? reg.end() - params_.splice_region_ex + 1 : reg.start() - params_.splice_region_in_3;
			int three_prime_reg_end = plus_strand ? reg.end() + params_.splice_region_in_3 : reg.start() + params_.splice_region_ex - 1;
			int donor_reg_start = plus_strand ? reg.end()+1 : reg.start()-2;
			int donor_reg_end = plus_strand ? reg.end()+2 : reg.start()-1;

			if (!insertion)
			{
				if (BasicStatistics::rangeOverlaps(start, end, three_prime_reg_start, three_prime_reg_end))
				{
					if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in 3' splice region :" << three_prime_reg_start << "-" << three_prime_reg_end;
					hgvs.types.insert(VariantConsequenceType::SPLICE_REGION_VARIANT);

					if (BasicStatistics::rangeOverlaps(start, end, donor_reg_start, donor_reg_end))
					{
						if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in splice donor: " << donor_reg_start << "-" << donor_reg_end;
						hgvs.types.insert(VariantConsequenceType::SPLICE_DONOR_VARIANT);
					}
				}
			}
			else //special handling for insertions (they affect no base directly)
			{
				if (BasicStatistics::rangeOverlaps(start, start+1, three_prime_reg_start+1, three_prime_reg_end-1))
				{
					if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in 3' splice region :" << three_prime_reg_start << "-" << three_prime_reg_end;
					hgvs.types.insert(VariantConsequenceType::SPLICE_REGION_VARIANT);

					if (start==donor_reg_start)
					{
						if (debug) qDebug() << "annotateSpliceRegion" << __LINE__ << "in splice donor: " << donor_reg_start << "-" << donor_reg_end;
						hgvs.types.insert(VariantConsequenceType::SPLICE_DONOR_VARIANT);
					}
				}
			}
		}

		//splice donor/acceptor variant has unknown consequences on protein level
		if(transcript.isCoding() && (hgvs.types.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT) || hgvs.types.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT)))
		{
			hgvs.hgvs_p = "p.?";
		}
    }
}

//extract the coding DNA sequence of the transcript from the reference genome
Sequence VariantHgvsAnnotator::getCodingSequence(const Transcript& trans, bool add_utr_3)
{
    Sequence seq;

    if(add_utr_3 && trans.strand() == Transcript::MINUS)
    {
        for(int i=0; i<trans.utr3prime().count(); i++)
        {
            int start = trans.utr3prime()[i].start();
            int length = trans.utr3prime()[i].end() - start + 1;
			seq.append(genome_idx_.seq(trans.chr(), start, length));
        }
        //incomplete 3' end: add 30 bases from the reference sequence
        if(trans.utr3prime().count() == 0)
        {
			seq.append(genome_idx_.seq(trans.chr(), std::max(trans.start() - 30, 1), std::min(30, trans.start() - 1)));
        }
    }

    for(int i=0; i<trans.codingRegions().count(); i++)
    {
        int start = trans.codingRegions()[i].start();
        int length = trans.codingRegions()[i].end() - start + 1;
		seq.append(genome_idx_.seq(trans.chr(), start, length));
    }

    if(add_utr_3 && trans.strand() == Transcript::PLUS)
    {
        for(int i=0; i<trans.utr3prime().count(); i++)
        {
            int start = trans.utr3prime()[i].start();
            int length = trans.utr3prime()[i].end() - start + 1;
			seq.append(genome_idx_.seq(trans.chr(), start, length));
        }
        //incomplete 3' end: add 30 bases from the reference sequence
        if(trans.utr3prime().count() == 0)
        {
			seq.append(genome_idx_.seq(trans.chr(), trans.end() + 1, 30));
        }
    }

    if(trans.strand() == Transcript::MINUS)
    {
        seq.reverseComplement();
    }

    return seq;
}

//Returns the variant types as string (ordered alphabetically)
QByteArray VariantConsequence::typesToString(QByteArray sep) const
{
	QByteArrayList output;

	foreach(const VariantConsequenceType& type, types)
	{
		output << VariantConsequence::typeToString(type);
	}

	std::sort(output.begin(), output.end());

	return output.join(sep);
}

QByteArray VariantConsequence::toString() const
{
	QByteArray output = "HGVS.c=" + hgvs_c + " HGVS.p=" + hgvs_p + " impact=" + impact + " types=" + typesToString();

	if (exon_number!=-1) output += " exon=" + QByteArray::number(exon_number);
	if (intron_number!=-1) output += " intron=" + QByteArray::number(intron_number);

	return output;
}

VariantHgvsAnnotator::Parameters::Parameters(int max_dist_trans, int splice_reg_exon, int splice_reg_intron_5, int splice_reg_intron_3)
	: max_dist_to_transcript(max_dist_trans)
	, splice_region_ex(splice_reg_exon)
	, splice_region_in_5(splice_reg_intron_5)
	, splice_region_in_3(splice_reg_intron_3)
{
}

void VariantHgvsAnnotator::annotateExonIntronNumber(VariantConsequence& hgvs, const Transcript& transcript, const VcfLine& variant, bool debug)
{
	const BedFile& regions = transcript.regions();
	bool plus_strand = transcript.isPlusStrand();
	bool insertion = variant.isIns();

	//determine affected start/end
	int start = variant.start();
	int end = variant.end();
	if (variant.isDel() || variant.isInDel()) //deletion/deletion-insertion: change is after prefix base > adjust start (end is correct)
	{
		start += 1;
	}
	if (debug) qDebug() << "annotateExonIntronNumber" << start << "-" << end << insertion;

	//exon number (insertions between intron and exon count as exonic)
	if (plus_strand)
	{
		for(int i=0; i<regions.count(); ++i)
		{
			if((!insertion && regions[i].overlapsWith(start, end)) || (insertion && BasicStatistics::rangeOverlaps(start, start+1, regions[i].start(), regions[i].end())))
			{
				hgvs.exon_number = i + 1;
				break;
			}
		}
	}
	else
	{
		for(int i=regions.count()-1; i>=0; --i)
		{
			if((!insertion && regions[i].overlapsWith(start, end)) || (insertion && BasicStatistics::rangeOverlaps(start, start+1, regions[i].start(), regions[i].end())))
			{
				hgvs.exon_number = regions.count() - i;
				break;
			}
		}
	}

	//intron (insertions between intron and exon count as exonic)
	if(plus_strand)
	{
		for(int i=0; i<regions.count()-1; ++i)
		{
			if((!insertion && BasicStatistics::rangeOverlaps(start, end, regions[i].end()+1, regions[i+1].start()-1)) || (insertion && BasicStatistics::rangeOverlaps(start, start+1, regions[i].end()+2, regions[i+1].start()-2)))
			{
				hgvs.intron_number = i + 1;
				break;
			}
		}
	}
	else
	{
		for(int i=regions.count()-2; i>=0; --i)
		{
			if((!insertion && BasicStatistics::rangeOverlaps(start, end, regions[i].end()+1, regions[i+1].start()-1)) || (insertion && BasicStatistics::rangeOverlaps(start, start+1, regions[i].end()+2, regions[i+1].start()-2)))
			{
				hgvs.intron_number = regions.count() - i - 1;
				break;
			}
		}
	}
}
