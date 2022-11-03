#ifndef VARIANTHGVSANNOTATOR_H
#define VARIANTHGVSANNOTATOR_H

#include "cppNGS_global.h"
#include "BedFile.h"
#include "VariantList.h"
#include "VcfFile.h"
#include "FastaFileIndex.h"
#include "Transcript.h"
#include "Sequence.h"
#include "NGSHelper.h"
#include "Exceptions.h"

///Representation of the effect of a variant
enum class VariantConsequenceType : int
{
    INTERGENIC_VARIANT,
    DOWNSTREAM_GENE_VARIANT,
    UPSTREAM_GENE_VARIANT,
    NON_CODING_TRANSCRIPT_VARIANT,
    INTRON_VARIANT,
    NON_CODING_TRANSCRIPT_EXON_VARIANT,
    THREE_PRIME_UTR_VARIANT,
    FIVE_PRIME_UTR_VARIANT,
    CODING_SEQUENCE_VARIANT,
    SYNONYMOUS_VARIANT,
    STOP_RETAINED_VARIANT,
    START_RETAINED_VARIANT,
    INCOMPLETE_TERMINAL_CODON_VARIANT,
    SPLICE_REGION_VARIANT,
    PROTEIN_ALTERING_VARIANT,
    MISSENSE_VARIANT,
    INFRAME_DELETION,
    INFRAME_INSERTION,
    START_LOST,
    STOP_LOST,
    FRAMESHIFT_VARIANT,
    STOP_GAINED,
    SPLICE_DONOR_VARIANT,
	SPLICE_ACCEPTOR_VARIANT,
	NMD_TRANSCRIPT_VARIANT
};

inline uint qHash(VariantConsequenceType key)
{
	return qHash(static_cast<int>(key));
}

///Representation of HGVS nomenclature
struct CPPNGSSHARED_EXPORT VariantConsequence
{
	QByteArray allele; //TODO Move to VcfAnnotateConsequence > MARC
	QByteArray hgvs_c;
	QByteArray hgvs_p;
	QSet<VariantConsequenceType> types;
	QByteArray impact;
    int exon_number{-1};
    int intron_number{-1};

	QByteArray typesToString(QByteArray sep="&");
	static QByteArray typeToString(VariantConsequenceType type)
    {
        switch(type)
        {
            case VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT: return "splice_acceptor_variant";
            case VariantConsequenceType::SPLICE_DONOR_VARIANT: return "splice_donor_variant";
            case VariantConsequenceType::STOP_GAINED: return "stop_gained";
            case VariantConsequenceType::FRAMESHIFT_VARIANT: return "frameshift_variant";
            case VariantConsequenceType::STOP_LOST: return "stop_lost";
            case VariantConsequenceType::START_LOST: return "start_lost";
            case VariantConsequenceType::INFRAME_INSERTION: return "inframe_insertion";
            case VariantConsequenceType::INFRAME_DELETION: return "inframe_deletion";
            case VariantConsequenceType::MISSENSE_VARIANT: return "missense_variant";
            case VariantConsequenceType::PROTEIN_ALTERING_VARIANT: return "protein_altering_variant";
            case VariantConsequenceType::SPLICE_REGION_VARIANT: return "splice_region_variant";
            case VariantConsequenceType::INCOMPLETE_TERMINAL_CODON_VARIANT: return "incomplete_terminal_codon_variant";
            case VariantConsequenceType::START_RETAINED_VARIANT: return "start_retained_variant";
            case VariantConsequenceType::STOP_RETAINED_VARIANT: return "stop_retained_variant";
            case VariantConsequenceType::SYNONYMOUS_VARIANT: return "synonymous_variant";
            case VariantConsequenceType::CODING_SEQUENCE_VARIANT: return "coding_sequence_variant";
            case VariantConsequenceType::FIVE_PRIME_UTR_VARIANT: return "5_prime_UTR_variant";
            case VariantConsequenceType::THREE_PRIME_UTR_VARIANT: return "3_prime_UTR_variant";
            case VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT: return "non_coding_transcript_exon_variant";
            case VariantConsequenceType::INTRON_VARIANT: return "intron_variant";
            case VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT: return "non_coding_transcript_variant";
            case VariantConsequenceType::UPSTREAM_GENE_VARIANT: return "upstream_gene_variant";
            case VariantConsequenceType::DOWNSTREAM_GENE_VARIANT: return "downstream_gene_variant";
            case VariantConsequenceType::INTERGENIC_VARIANT: return "intergenic_variant";
			case VariantConsequenceType::NMD_TRANSCRIPT_VARIANT: return "NMD_transcript_variant";

		}

		THROW(ProgrammingException, "Unhandled variant consequence type " + QByteArray::number(static_cast<int>(type)) + "!");
    }
};

///Class for generating HGVS nomenclature and variant effect from VCF/GSVar
class CPPNGSSHARED_EXPORT VariantHgvsAnnotator
{
public:
    ///Constructor to change parameters for detecting up/downstream and splice region variants: different for 5 and 3 prime site intron
	VariantHgvsAnnotator(const FastaFileIndex& genome_idx, int max_dist_to_transcript=5000, int splice_region_ex=3, int splice_region_in_5=20, int splice_region_in_3=20); //TODO move parameters to struct > MARC

	///Calculates variant consequence from VCF-style variant (not multi-allelic)
	VariantConsequence annotate(const Transcript& transcript, VcfLine& variant);
	///Calculates variant consequence from GSvar-style variant
	VariantConsequence annotate(const Transcript& transcript, const Variant& variant);

	///Converts consequence type to impact
	static QByteArray consequenceTypeToImpact(VariantConsequenceType type);

private:

	QByteArray translate(const Sequence& seq, bool is_mito = false, bool end_at_stop = true);
	Sequence getCodingSequence(const Transcript& trans, bool add_utr_3 = false);

	const FastaFileIndex& genome_idx_;
	//How far upstream or downstream can the variant be from the transcript
	int max_dist_to_transcript_;

	//How far the splice region extends into exon/intron
	int splice_region_ex_;
	int splice_region_in_5_;
	int splice_region_in_3_;

	QByteArray annotateRegionsCoding(const Transcript& transcript, VariantConsequence& hgvs, int gen_pos, bool is_dup = false);
	QByteArray annotateRegionsNonCoding(const Transcript& transcript, VariantConsequence& hgvs, int gen_pos, bool is_dup = false);
	QByteArray getHgvsPosition(const BedFile& regions, VariantConsequence& hgvs, int gen_pos, bool plus_strand, const BedFile& coding_regions, bool utr_5 = false, int first_region = 0);
	QByteArray getPositionInIntron(const BedFile& regions, VariantConsequence& hgvs, int genomic_position, bool plus_strand, const BedFile &coding_regions, bool utr_5 = false, int first_region = 0);
	QByteArray getHgvsProteinAnnotation(const VcfLine& variant, const QByteArray& pos_hgvs_c, const Transcript& transcript);

	void annotateSpliceRegion(VariantConsequence& hgvs, const Transcript& transcript, int start, int end, bool insertion);

	void annotateProtSeqCsqSnv(VariantConsequence& hgvs);

};

#endif // VARIANTHGVSANNOTATOR_H
