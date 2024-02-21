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
#include "VariantImpact.h"

///Representation of the effect of a variant
///NOTE: the order is important as it defines the severity of the variant.
enum class VariantConsequenceType : int
{
    INTERGENIC_VARIANT,
    DOWNSTREAM_GENE_VARIANT,
    UPSTREAM_GENE_VARIANT,
	NMD_TRANSCRIPT_VARIANT,
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
	STOP_GAINED,
	FRAMESHIFT_VARIANT,
    SPLICE_DONOR_VARIANT,
	SPLICE_ACCEPTOR_VARIANT
};

inline uint qHash(VariantConsequenceType key)
{
	return qHash(static_cast<int>(key));
}

///Representation of HGVS nomenclature
struct CPPNGSSHARED_EXPORT VariantConsequence
{
	QByteArray hgvs_c;
	QByteArray hgvs_p;
	QSet<VariantConsequenceType> types;
	VariantImpact impact;
    int exon_number{-1};
    int intron_number{-1};

	QByteArray normalized; //normalized VCF representation after shifting according to 3' rule

	QByteArray typesToString(QByteArray sep="&") const;
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

	QByteArray toString() const;
};

///Class for generating HGVS nomenclature and variant effect from VCF/GSVar
class CPPNGSSHARED_EXPORT VariantHgvsAnnotator
{
public:

	///Parameters struct
	struct CPPNGSSHARED_EXPORT Parameters
	{
		Parameters(int max_dist_trans = 5000, int splice_reg_exon=3, int splice_reg_intron_5=20, int splice_reg_intron_3=20);

		int max_dist_to_transcript; //Max distance of variant from transcript to be annotated with information about that transcript.
		int splice_region_ex; //Number of exonic bases flanking the splice junction that are annotated as splice region.
		int splice_region_in_5; //Number of introninc bases flanking the 5' splice junction that are annotated as splice region.
		int splice_region_in_3; //Number of introninc bases flanking the 3' splice junction that are annotated as splice region.
	};

    ///Constructor to change parameters for detecting up/downstream and splice region variants: different for 5 and 3 prime site intron
	VariantHgvsAnnotator(const FastaFileIndex& genome_idx, Parameters params = Parameters());

	///Calculates variant consequence from VCF-style variant (not multi-allelic)
	VariantConsequence annotate(const Transcript& transcript, const VcfLine& variant, bool debug=false);
	///Calculates variant consequence from GSvar-style variant
	VariantConsequence annotate(const Transcript& transcript, const Variant& variant, bool debug=false);

	///Converts consequence type to impact
	static VariantImpact consequenceTypeToImpact(VariantConsequenceType type);

private:
	Parameters params_;
	const FastaFileIndex& genome_idx_;

	QByteArray annotateRegionsCoding(const Transcript& transcript, VariantConsequence& hgvs, int gen_pos, bool is_dup, bool debug=false);
	QByteArray annotateRegionsNonCoding(const Transcript& transcript, VariantConsequence& hgvs, int gen_pos, bool is_dup = false);
	QByteArray getHgvsPosition(const BedFile& regions, int gen_pos, bool plus_strand, const BedFile& coding_regions, bool utr_5 = false);
	QByteArray getPositionInIntron(const BedFile& regions, int genomic_position, bool plus_strand, const BedFile &coding_regions, bool utr_5 = false);
	QByteArray getHgvsProteinAnnotation(const VcfLine& variant, const QByteArray& pos_hgvs_c, const Transcript& transcript, bool debug=false);

	void annotateExonIntronNumber(VariantConsequence& hgvs, const Transcript& transcript, const VcfLine& variant, bool debug=false);

	void annotateSpliceRegion(VariantConsequence& hgvs, const Transcript& transcript, int start, int end, bool insertion, bool debug=false);

	void annotateProtSeqCsqSnv(VariantConsequence& hgvs);

	QByteArray translate(const Sequence& seq, bool is_mito = false, bool end_at_stop = true);

	Sequence getCodingSequence(const Transcript& trans, bool add_utr_3 = false);
};

#endif // VARIANTHGVSANNOTATOR_H
